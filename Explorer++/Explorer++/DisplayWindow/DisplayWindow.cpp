// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

/*
 * Creates and manages a 'display window'. This window
 * can be used to show various information (e.g. file
 * information) and is drawn with a gradient background.
 *
 * Icon:
 * The icon passed in when this class is created will be
 * drawn at 48x48 pixels. To avoid drawing issues, the
 * icon itself should be 48x48 pixels.
 *
 * Text drawing:
 * Text will be drawn in columns. Text height (and width)
 * is calculated, and once a particular column is full,
 * text will be placed in the next column.
 */

#include "stdafx.h"
#include "DisplayWindow.h"
#include "AppServices.h"
#include "AsyncIconFetcher.h"
#include "BrowserWindow.h"
#include "Config.h"
#include "DisplayWindowContextMenu.h"
#include "MainResource.h"
#include "PopupMenuView.h"
#include "ShellBrowser/NavigationEvents.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellBrowserEvents.h"
#include "TabEvents.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/WindowHelper.h"
#include "../Helper/WindowSubclass.h"

/* Defines how close the text can get to the bottom
of the display window before it is moved into the
next column. */
#define TEXT_VERTICAL_THRESHOLD 10

/* Defines the horizontal spacing between text columns. */
#define TEXT_COLUMN_SPACING 50

#define MAIN_ICON_LEFT 20
#define MAIN_ICON_TOP 20
#define MAIN_ICON_SIZE 48
#define ICON_TEXT_SPACING 12

/* Defines the position of thumbnails
for image files. The y-coordinate of
the image origin, but the x-coordinate
can float. */
#define THUMB_IMAGE_TOP 10

/* Determines how much spacing there is
at the top and bottom of the thumbnail. */
#define THUMB_HEIGHT_DELTA 20

std::list<ThumbnailEntry_t> g_ThumbnailEntries;

DisplayWindow *DisplayWindow::Create(HWND parent, Mode mode, BrowserWindow *browser,
	AppServices *appServices, Config *config)
{
	return new DisplayWindow(parent, mode, browser, appServices, config);
}

DisplayWindow::DisplayWindow(HWND parent, Mode mode, BrowserWindow *browser,
	AppServices *appServices, Config *config) :
	m_hwnd(CreateDisplayWindow(parent)),
	m_mode(mode),
	m_browser(browser),
	m_iconFetcher(appServices->GetAsyncIconFetcher()),
	m_config(config),
	m_acceleratorManager(appServices->GetAcceleratorManager()),
	m_resourceLoader(appServices->GetResourceLoader()),
	m_dpi(DpiCompatibility::GetInstance().GetDpiForWindow(m_hwnd))
{
	OnFontConfigChanged();

	m_bSizing = FALSE;
	m_hbmThumbnail = nullptr;
	m_bShowThumbnail = FALSE;
	m_bThumbnailExtracted = FALSE;
	m_bThumbnailExtractionFailed = FALSE;

	InitializeCriticalSection(&m_csDWThumbnails);

	m_windowSubclasses.push_back(std::make_unique<WindowSubclass>(m_hwnd,
		std::bind_front(&DisplayWindow::DisplayWindowProc, this)));

	m_connections.push_back(m_config->displayWindowCentreColor.addObserver(
		std::bind(&DisplayWindow::InvalidateWindow, this)));
	m_connections.push_back(m_config->displayWindowSurroundColor.addObserver(
		std::bind(&DisplayWindow::InvalidateWindow, this)));
	m_connections.push_back(m_config->displayWindowFont.addObserver(
		std::bind(&DisplayWindow::OnFontConfigChanged, this)));
	m_connections.push_back(m_config->displayWindowTextColor.addObserver(
		std::bind(&DisplayWindow::InvalidateWindow, this)));
	m_connections.push_back(m_config->displayWindowVertical.addObserver(
		std::bind(&DisplayWindow::InvalidateWindow, this)));

	FAIL_FAST_IF_FAILED(SHGetImageList(SHIL_EXTRALARGE, IID_PPV_ARGS(&m_systemImageList)));

	if (m_mode == Mode::Normal)
	{
		m_connections.push_back(appServices->GetTabEvents()->AddSelectedObserver(
			std::bind(&DisplayWindow::InvalidateWindow, this),
			TabEventScope::ForBrowser(*m_browser)));

		m_connections.push_back(
			appServices->GetShellBrowserEvents()->AddDirectoryPropertiesChangedObserver(
				std::bind(&DisplayWindow::InvalidateWindow, this),
				ShellBrowserEventScope::ForActiveShellBrowser(*m_browser)));

		m_connections.push_back(appServices->GetNavigationEvents()->AddCommittedObserver(
			std::bind(&DisplayWindow::InvalidateWindow, this),
			NavigationEventScope::ForActiveShellBrowser(*m_browser)));
	}
}

DisplayWindow::~DisplayWindow()
{
	DeleteCriticalSection(&m_csDWThumbnails);
}

HWND DisplayWindow::CreateDisplayWindow(HWND parent)
{
	static bool classRegistered = false;

	if (!classRegistered)
	{
		auto res = RegisterDisplayWindowClass();
		CHECK(res);

		classRegistered = true;
	}

	HWND displayWindow = CreateWindow(WINDOW_NAME, L"", WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS, 0,
		0, 0, 0, parent, nullptr, GetModuleHandle(nullptr), nullptr);
	CHECK(displayWindow);

	return displayWindow;
}

ATOM DisplayWindow::RegisterDisplayWindowClass()
{
	WNDCLASS windowClass = {};
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = DefWindowProc;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = GetModuleHandle(nullptr);
	windowClass.hIcon = nullptr;
	windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	windowClass.hbrBackground = nullptr;
	windowClass.lpszMenuName = nullptr;
	windowClass.lpszClassName = CLASS_NAME;
	return RegisterClass(&windowClass);
}

LRESULT DisplayWindow::DisplayWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_DPICHANGED_AFTERPARENT:
		OnDpiChanged();
		break;

	case WM_LBUTTONUP:
		m_bSizing = FALSE;
		ReleaseCapture();
		break;

	case WM_MOUSEMOVE:
		return OnMouseMove(lParam);

	case WM_LBUTTONDOWN:
		OnLButtonDown(lParam);
		break;

	case WM_CONTEXTMENU:
		OnShowContextMenu({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });
		break;

	case WM_ERASEBKGND:
		return 1;

	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc;
		RECT updateRect;
		RECT rc;

		GetUpdateRect(hwnd, &updateRect, FALSE);
		GetClientRect(hwnd, &rc);

		hdc = BeginPaint(hwnd, &ps);

		Draw(hdc, &rc, &updateRect);

		EndPaint(hwnd, &ps);
	}
	break;

	case DWM_BUFFERTEXT:
	{
		LineData_t ld;
		TCHAR *pszText = nullptr;

		pszText = (TCHAR *) lParam;

		if (pszText != nullptr)
		{
			StringCchCopy(ld.szText, std::size(ld.szText), pszText);

			m_LineList.push_back(ld);
		}

		/* TODO: Optimize? */
		InvalidateWindow();
	}
	break;

	case DWM_CLEARTEXTBUFFER:
	{
		m_LineList.clear();
	}
	break;

	case DWM_SETLINE:
	{
		TCHAR *pszText = nullptr;
		std::vector<LineData_t>::iterator itr;
		unsigned int iLine;
		unsigned int i = 0;

		iLine = (int) wParam;
		pszText = (TCHAR *) lParam;

		if (pszText != nullptr && iLine < m_LineList.size())
		{
			for (itr = m_LineList.begin(); itr != m_LineList.end(); itr++)
			{
				if (i == iLine)
				{
					StringCchCopy(itr->szText, std::size(itr->szText), pszText);
					break;
				}

				i++;
			}
		}

		/* TODO: Optimize? */
		InvalidateWindow();
	}
	break;

	case DWM_SETTHUMBNAILFILE:
		OnSetThumbnailFile(wParam, lParam);
		InvalidateWindow();
		break;

	case WM_NCDESTROY:
		delete this;
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

HWND DisplayWindow::GetHWND() const
{
	return m_hwnd;
}

int DisplayWindow::GetMinimumHeight() const
{
	auto dc = wil::GetDC(m_hwnd);
	auto selectFont = wil::SelectObject(dc.get(), m_font.get());
	TEXTMETRIC metrics;
	CHECK(GetTextMetrics(dc.get(), &metrics));
	int bottomPadding = MulDiv(TEXT_VERTICAL_THRESHOLD, m_dpi, USER_DEFAULT_SCREEN_DPI);
	auto iconRect = GetIconRect();
	return std::max(iconRect.bottom, iconRect.top + metrics.tmHeight) + bottomPadding;
}

void DisplayWindow::InvalidateWindow()
{
	RedrawWindow(m_hwnd, nullptr, nullptr, RDW_INVALIDATE);
}

void DisplayWindow::OnFontConfigChanged()
{
	auto font = m_config->displayWindowFont.get();
	font.lfHeight = MulDiv(font.lfHeight, m_dpi, USER_DEFAULT_SCREEN_DPI);
	font.lfWidth = MulDiv(font.lfWidth, m_dpi, USER_DEFAULT_SCREEN_DPI);
	m_font.reset(CreateFontIndirect(&font));
	CHECK(m_font);
	InvalidateWindow();
}

void DisplayWindow::OnDpiChanged()
{
	m_dpi = DpiCompatibility::GetInstance().GetDpiForWindow(m_hwnd);
	OnFontConfigChanged();
}

RECT DisplayWindow::GetIconRect() const
{
	int left = MulDiv(MAIN_ICON_LEFT, m_dpi, USER_DEFAULT_SCREEN_DPI);
	int top = MulDiv(MAIN_ICON_TOP, m_dpi, USER_DEFAULT_SCREEN_DPI);
	int size = MulDiv(MAIN_ICON_SIZE, m_dpi, USER_DEFAULT_SCREEN_DPI);
	return { left, top, left + size, top + size };
}

void DisplayWindow::Draw(HDC hdc, RECT *rc, RECT *updateRect)
{
	HDC hdcMem = CreateCompatibleDC(hdc);
	HBITMAP hBitmap = CreateCompatibleBitmap(hdc, rc->right - rc->left, rc->bottom - rc->top);
	HGDIOBJ hOriginalObject = SelectObject(hdcMem, hBitmap);

	DrawBackground(hdcMem, rc);

	PaintText(hdcMem);

	int folderIconIndex;

	if (m_mode == Mode::Normal)
	{
		auto *shellBrowser = m_browser->GetActiveShellBrowser();
		folderIconIndex =
			m_iconFetcher->GetCachedIconIndexOrDefault(shellBrowser->GetDirectory().Raw());
	}
	else
	{
		folderIconIndex = m_iconFetcher->GetDefaultIconIndex(DefaultIconType::Folder);
	}

	wil::unique_hicon icon;
	FAIL_FAST_IF_FAILED(m_systemImageList->GetIcon(folderIconIndex, ILD_TRANSPARENT, &icon));
	auto iconRect = GetIconRect();
	auto res = DrawIconEx(hdcMem, iconRect.left, iconRect.top, icon.get(), GetRectWidth(&iconRect),
		GetRectHeight(&iconRect), 0, nullptr, DI_NORMAL);
	DCHECK(res);

	if (m_bShowThumbnail)
	{
		DrawThumbnail(hdcMem);
	}

	BitBlt(hdc, updateRect->left, updateRect->top, rc->right, rc->bottom, hdcMem, updateRect->left,
		updateRect->top, SRCCOPY);

	SelectObject(hdcMem, hOriginalObject);
	DeleteObject(hBitmap);
	DeleteDC(hdcMem);
}

void DisplayWindow::DrawBackground(HDC hdcMem, RECT *rc)
{
	Gdiplus::Graphics graphics(hdcMem);

	Gdiplus::Rect displayRect(0, 0, rc->right - rc->left, rc->bottom - rc->top);

	Gdiplus::GraphicsPath path;
	path.AddRectangle(displayRect);
	Gdiplus::PathGradientBrush pgb(&path);
	pgb.SetCenterPoint(Gdiplus::Point(0, 0));

	Gdiplus::Color centreColor;
	centreColor.SetFromCOLORREF(m_config->displayWindowCentreColor.get());
	pgb.SetCenterColor(centreColor);

	Gdiplus::Color surroundColor;
	surroundColor.SetFromCOLORREF(m_config->displayWindowSurroundColor.get());
	INT count = 1;
	pgb.SetSurroundColors(&surroundColor, &count);
	graphics.FillRectangle(&pgb, displayRect);

	/* This draws a separator line across the top edge of the window,
	so that it is visually separated from other windows. */
	Gdiplus::Pen newPen(BORDER_COLOUR, 1);

	if (m_config->displayWindowVertical.get())
	{
		graphics.DrawLine(&newPen, 0, 0, 0, rc->bottom);
	}
	else
	{
		graphics.DrawLine(&newPen, 0, 0, rc->right, 0);
	}
}

void DisplayWindow::DrawThumbnail(HDC hdcMem)
{
	if (!m_bThumbnailExtracted)
	{
		ExtractThumbnailImage();
	}
	else
	{
		if (!m_bThumbnailExtractionFailed)
		{
			RECT rc;
			GetClientRect(m_hwnd, &rc);

			HDC hdcSrc = CreateCompatibleDC(hdcMem);
			auto hBitmapOld = (HBITMAP) SelectObject(hdcSrc, m_hbmThumbnail);

			BitBlt(hdcMem, m_xColumnFinal, THUMB_IMAGE_TOP, GetRectWidth(&rc) - m_xColumnFinal,
				GetRectHeight(&rc) - THUMB_HEIGHT_DELTA, hdcSrc, 0, 0, SRCCOPY);

			SelectObject(hdcSrc, hBitmapOld);
			DeleteDC(hdcSrc);
		}
	}
}

DWORD WINAPI Thread_ExtractThumbnailImage(LPVOID lpParameter)
{
	DisplayWindow *pdw = nullptr;

	pdw = (DisplayWindow *) ((ThumbnailEntry_t *) lpParameter)->pdw;

	CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

	pdw->ExtractThumbnailImageInternal((ThumbnailEntry_t *) lpParameter);

	/* TODO: Erase the item. */
	// g_ThumbnailEntries.erase();

	CoUninitialize();

	return 0;
}

void DisplayWindow::ExtractThumbnailImage()
{
	ThumbnailEntry_t te;

	te.pdw = (void *) this;
	te.bCancelled = FALSE;
	g_ThumbnailEntries.push_back(te);

	HANDLE hThread = CreateThread(nullptr, 0, Thread_ExtractThumbnailImage,
		(LPVOID) &g_ThumbnailEntries.back(), 0, nullptr);
	CloseHandle(hThread);
}

void DisplayWindow::ExtractThumbnailImageInternal(ThumbnailEntry_t *pte)
{
	IExtractImage *pExtractImage = nullptr;
	IShellFolder *pShellFolder = nullptr;
	PIDLIST_ABSOLUTE pidlParent = nullptr;
	PIDLIST_ABSOLUTE pidlFull = nullptr;
	PITEMID_CHILD pridl = nullptr;
	HBITMAP hBitmap;
	BITMAP bm;
	RECT rc;
	SIZE size;
	TCHAR szImage[MAX_PATH];
	DWORD dwPriority;
	DWORD dwFlags;
	HRESULT hr;

	m_bThumbnailExtracted = TRUE;
	m_bThumbnailExtractionFailed = TRUE;

	hr = SHParseDisplayName(m_ImageFile, nullptr, &pidlFull, 0, nullptr);

	if (SUCCEEDED(hr))
	{
		pidlParent = ILCloneFull(pidlFull);
		ILRemoveLastID(pidlParent);

		pridl = ILCloneChild(ILFindLastID(pidlFull));

		hr = SHBindToObject(nullptr, pidlParent, nullptr, IID_PPV_ARGS(&pShellFolder));

		if (SUCCEEDED(hr))
		{
			hr = GetUIObjectOf(pShellFolder, nullptr, 1, &pridl, IID_PPV_ARGS(&pExtractImage));

			if (SUCCEEDED(hr))
			{
				GetClientRect(m_hwnd, &rc);

				/* First, query the thumbnail so that its actual aspect
				ratio can be calculated. */
				dwFlags = IEIFLAG_OFFLINE | IEIFLAG_QUALITY | IEIFLAG_ORIGSIZE;
				size.cx = GetRectHeight(&rc) - THUMB_HEIGHT_DELTA;
				size.cy = GetRectHeight(&rc) - THUMB_HEIGHT_DELTA;

				hr = pExtractImage->GetLocation(szImage, std::size(szImage), &dwPriority, &size, 32,
					&dwFlags);

				if (SUCCEEDED(hr))
				{
					hr = pExtractImage->Extract(&hBitmap);

					if (SUCCEEDED(hr))
					{
						/* Get bitmap information (including height and width). */
						GetObject(hBitmap, sizeof(BITMAP), &bm);

						/* Delete the original bitmap. */
						DeleteObject(hBitmap);

						/* ...now query the thumbnail again, this time adjusting
						the width of the suggested area based on the actual aspect
						ratio. */
						dwFlags =
							IEIFLAG_OFFLINE | IEIFLAG_QUALITY | IEIFLAG_ASPECT | IEIFLAG_ORIGSIZE;
						size.cy = GetRectHeight(&rc) - THUMB_HEIGHT_DELTA;
						size.cx = (LONG) ((double) size.cy
							* ((double) bm.bmWidth / (double) bm.bmHeight));
						m_iImageWidth = size.cx;
						m_iImageHeight = size.cy;
						pExtractImage->GetLocation(szImage, std::size(szImage), &dwPriority, &size,
							32, &dwFlags);
						hr = pExtractImage->Extract(&m_hbmThumbnail);

						if (SUCCEEDED(hr))
						{
							/* Check first if we've been cancelled. This might happen,
							for example, if another file is selected while the current
							thumbnail is been found. */
							EnterCriticalSection(&m_csDWThumbnails);

							if (!pte->bCancelled)
							{
								m_bThumbnailExtractionFailed = FALSE;
								InvalidateRect(m_hwnd, nullptr, FALSE);
							}

							LeaveCriticalSection(&m_csDWThumbnails);
						}
						else
						{
							m_bThumbnailExtractionFailed = TRUE;
							m_hbmThumbnail = nullptr;
						}
					}
				}

				pExtractImage->Release();
			}

			pShellFolder->Release();
		}

		CoTaskMemFree(pidlFull);
		CoTaskMemFree(pidlParent);
		CoTaskMemFree(pridl);
	}
}

std::vector<RECT> DisplayWindow::CalculateTextRects(HDC hdc)
{
	RECT rcClient;
	GetClientRect(m_hwnd, &rcClient);
	auto selectFont = wil::SelectObject(hdc, m_font.get());
	int topPadding = MulDiv(MAIN_ICON_TOP, m_dpi, USER_DEFAULT_SCREEN_DPI);
	int bottomPadding = MulDiv(TEXT_VERTICAL_THRESHOLD, m_dpi, USER_DEFAULT_SCREEN_DPI);
	int columnSpacing = MulDiv(TEXT_COLUMN_SPACING, m_dpi, USER_DEFAULT_SCREEN_DPI);
	int xCurrent = GetIconRect().right + MulDiv(ICON_TEXT_SPACING, m_dpi, USER_DEFAULT_SCREEN_DPI);
	int yCurrent = topPadding;
	int columnWidth = 0;
	std::vector<RECT> textRects;

	for (const auto &line : m_LineList)
	{
		SIZE stringSize;
		auto res = GetTextExtentPoint32(hdc, line.szText, lstrlen(line.szText), &stringSize);
		CHECK(res);

		if (yCurrent > topPadding && yCurrent + stringSize.cy > rcClient.bottom - bottomPadding)
		{
			xCurrent += columnWidth + columnSpacing;
			columnWidth = 0;
			yCurrent = topPadding;
		}

		columnWidth = std::max(columnWidth, static_cast<int>(stringSize.cx));
		textRects.push_back(
			{ xCurrent, yCurrent, xCurrent + stringSize.cx, yCurrent + stringSize.cy });
		yCurrent += stringSize.cy;
	}

	m_xColumnFinal = xCurrent + columnWidth + columnSpacing;
	return textRects;
}

void DisplayWindow::PaintText(HDC hdc)
{
	auto textRects = CalculateTextRects(hdc);
	auto selectFont = wil::SelectObject(hdc, m_font.get());
	SetBkMode(hdc, TRANSPARENT);
	SetTextColor(hdc, m_config->displayWindowTextColor.get());

	RECT clientRect;
	GetClientRect(m_hwnd, &clientRect);
	for (size_t i = 0; i < textRects.size(); i++)
	{
		auto &textRect = textRects[i];
		textRect.right = std::min(textRect.right, clientRect.right);
		if (textRect.left < textRect.right)
		{
			TransparentTextOut(hdc, m_LineList[i].szText, &textRect);
		}
	}
}

void DisplayWindow::TransparentTextOut(HDC hdc, TCHAR *text, RECT *prcText)
{
	DrawText(hdc, text, lstrlen(text), prcText,
		DT_LEFT | DT_NOPREFIX | DT_SINGLELINE | DT_END_ELLIPSIS);
}

LONG DisplayWindow::OnMouseMove(LPARAM lParam)
{
	POINT cursorPos;
	static POINT prevCursorPos;
	RECT rc;
	RECT rc2;

	cursorPos.x = GET_X_LPARAM(lParam);
	cursorPos.y = GET_Y_LPARAM(lParam);

	GetClientRect(m_hwnd, &rc);

	GetClientRect(GetParent(m_hwnd), &rc2);

	if (m_bSizing)
	{
		if ((prevCursorPos.x == cursorPos.x) && (prevCursorPos.y == cursorPos.y))
		{
			return 0;
		}

		prevCursorPos.x = cursorPos.x;
		prevCursorPos.y = cursorPos.y;

		/* Notify the main window, so that it can redraw/reposition
		its other windows. */
		SendMessage(GetParent(m_hwnd), WM_USER_DISPLAYWINDOWRESIZED,
			MAKEWPARAM(std::max(rc.right - cursorPos.x, 0L), std::max(rc.bottom - cursorPos.y, 0L)),
			0);
	}

	int resizeRange = MulDiv(5, m_dpi, USER_DEFAULT_SCREEN_DPI);
	if ((m_config->displayWindowVertical.get() && cursorPos.x <= (rc.left + resizeRange))
		|| (!m_config->displayWindowVertical.get() && cursorPos.y <= (rc.top + resizeRange)))
	{
		SetCursor(
			LoadCursor(NULL, m_config->displayWindowVertical.get() ? IDC_SIZEWE : IDC_SIZENS));
	}

	/* If there is a thumbnail preview
	for an image been shown, and the
	mouse is over the thumbnail, set the
	mouse pointer to a hand. */
	if (m_bShowThumbnail)
	{
		RECT rcThumbnail;

		SetRect(&rcThumbnail, m_xColumnFinal, THUMB_IMAGE_TOP, m_xColumnFinal + m_iImageWidth,
			THUMB_IMAGE_TOP + m_iImageHeight);

		if (PtInRect(&rcThumbnail, cursorPos))
		{
			SetCursor(LoadCursor(nullptr, IDC_HAND));
		}
	}

	return 1;
}

void DisplayWindow::OnLButtonDown(LPARAM lParam)
{
	POINT cursorPos;
	RECT rc;

	cursorPos.x = GET_X_LPARAM(lParam);
	cursorPos.y = GET_Y_LPARAM(lParam);

	GetClientRect(m_hwnd, &rc);

	int resizeRange = MulDiv(5, m_dpi, USER_DEFAULT_SCREEN_DPI);
	if ((m_config->displayWindowVertical.get() && cursorPos.x <= (rc.left + resizeRange))
		|| (!m_config->displayWindowVertical.get() && cursorPos.y <= (rc.top + resizeRange)))
	{
		SetCursor(
			LoadCursor(NULL, m_config->displayWindowVertical.get() ? IDC_SIZEWE : IDC_SIZENS));
		m_bSizing = TRUE;
		SetFocus(m_hwnd);
		SetCapture(m_hwnd);
	}

	/* If an image thumbnail was clicked, open
	the image and set the mouse pointer back to
	the regular pointer. */
	if (m_bShowThumbnail)
	{
		RECT rcThumbnail;

		SetRect(&rcThumbnail, m_xColumnFinal, THUMB_IMAGE_TOP, m_xColumnFinal + m_iImageWidth,
			THUMB_IMAGE_TOP + m_iImageHeight);

		if (PtInRect(&rcThumbnail, cursorPos))
		{
			/* TODO: Parent should be notified. */
			SetCursor(LoadCursor(nullptr, IDC_HAND));
			ShellExecute(m_hwnd, _T("open"), m_ImageFile, nullptr, nullptr, SW_SHOWNORMAL);
		}
	}
}

void DisplayWindow::OnShowContextMenu(const POINT &ptScreen)
{
	if (m_mode == Mode::Preview)
	{
		// If the display window is being previewed (e.g. to show what a color change would look
		// like), no context menus will be shown.
		return;
	}

	POINT ptClient = ptScreen;
	auto res = ScreenToClient(m_hwnd, &ptClient);
	CHECK(res);

	RECT rc = GetIconRect();

	if (PtInRect(&rc, ptClient))
	{
		// TODO: Show file context menu.
	}
	else
	{
		PopupMenuView popupMenu(m_hwnd, m_browser);
		DisplayWindowContextMenu contextMenu(&popupMenu, m_acceleratorManager, m_browser, m_config,
			m_resourceLoader);
		popupMenu.Show(ptScreen);
	}
}

void DisplayWindow::CancelThumbnailExtraction()
{
	std::list<ThumbnailEntry_t>::iterator itr;

	EnterCriticalSection(&m_csDWThumbnails);

	for (itr = g_ThumbnailEntries.begin(); itr != g_ThumbnailEntries.end(); itr++)
	{
		itr->bCancelled = TRUE;
	}

	LeaveCriticalSection(&m_csDWThumbnails);
}

void DisplayWindow::OnSetThumbnailFile(WPARAM wParam, LPARAM lParam)
{
	m_bShowThumbnail = (BOOL) lParam;

	if (m_bShowThumbnail)
	{
		CancelThumbnailExtraction();

		if (m_hbmThumbnail)
		{
			DeleteObject(m_hbmThumbnail);
		}

		m_iImageWidth = 0;
		m_iImageHeight = 0;
		m_bThumbnailExtracted = FALSE;
		m_bThumbnailExtractionFailed = FALSE;
		StringCchCopy(m_ImageFile, std::size(m_ImageFile), (TCHAR *) wParam);
	}
}
