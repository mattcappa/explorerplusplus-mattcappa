// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "SplitPaneDividerView.h"
#include "ThemeManager.h"
#include "../Helper/DoubleBufferedPaint.h"
#include "../Helper/WindowSubclass.h"
#include <windowsx.h>

SplitPaneDividerView::SplitPaneDividerView(ViewBase *parent, const ThemeManager *themeManager) :
	ViewBase(Create(parent)),
	m_themeManager(themeManager)
{
	AddSubclass(std::make_unique<WindowSubclass>(GetHWND(),
		std::bind_front(&SplitPaneDividerView::WndProc, this)));
}

HWND SplitPaneDividerView::Create(ViewBase *parent)
{
	static const auto windowClass = []
	{
		WNDCLASS wc = {};
		wc.lpfnWndProc = DefWindowProc;
		wc.hInstance = GetModuleHandle(nullptr);
		wc.hCursor = LoadCursor(nullptr, IDC_SIZEWE);
		wc.lpszClassName = L"Explorer++SplitPaneDivider";
		wc.style = CS_DBLCLKS;
		auto atom = RegisterClass(&wc);
		CHECK(atom);
		return atom;
	}();

	auto hwnd = CreateWindow(MAKEINTATOM(windowClass), L"", WS_CHILD | WS_CLIPSIBLINGS, 0, 0, 0, 0,
		parent->GetHWND(), nullptr, GetModuleHandle(nullptr), nullptr);
	CHECK(hwnd);
	return hwnd;
}

void SplitPaneDividerView::SetResizedCallback(std::function<void(double)> callback)
{
	m_resizedCallback = std::move(callback);
}

void SplitPaneDividerView::SetBounds(const RECT &contentBounds, const RECT &dividerBounds,
	bool visible)
{
	m_contentBounds = contentBounds;

	if (!visible && GetCapture() == GetHWND())
	{
		ReleaseCapture();
	}

	SetWindowPos(GetHWND(), nullptr, dividerBounds.left, dividerBounds.top,
		dividerBounds.right - dividerBounds.left, dividerBounds.bottom - dividerBounds.top,
		SWP_NOZORDER | SWP_NOACTIVATE | (visible ? SWP_SHOWWINDOW : SWP_HIDEWINDOW));
}

LRESULT SplitPaneDividerView::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_LBUTTONDOWN:
		m_dragOffset = GET_X_LPARAM(lParam);
		m_dragging = true;
		SetCapture(hwnd);
		return 0;

	case WM_MOUSEMOVE:
		OnMouseMove({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });
		return 0;

	case WM_LBUTTONUP:
	case WM_CANCELMODE:
		if (GetCapture() == hwnd)
		{
			ReleaseCapture();
		}
		return 0;

	case WM_CAPTURECHANGED:
		m_dragging = false;
		return 0;

	case WM_LBUTTONDBLCLK:
		if (m_resizedCallback)
		{
			m_resizedCallback(0.5);
		}
		return 0;

	case WM_ERASEBKGND:
		return 1;

	case WM_PAINT:
	{
		DoubleBufferedPaint paint(hwnd);
		Paint(paint.GetMemoryDC());
	}
		return 0;

	case WM_PRINTCLIENT:
		Paint(reinterpret_cast<HDC>(wParam));
		return 0;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void SplitPaneDividerView::Paint(HDC hdc)
{
	RECT rect;
	GetClientRect(GetHWND(), &rect);
	auto *colors = m_themeManager->GetCurrentColorProvider();
	FillRect(hdc, &rect, colors->GetBorderBrush());
}

void SplitPaneDividerView::OnMouseMove(const POINT &pt)
{
	if (!m_dragging || !m_resizedCallback)
	{
		return;
	}

	RECT clientRect;
	GetClientRect(GetHWND(), &clientRect);
	auto availableWidth =
		m_contentBounds.right - m_contentBounds.left - (clientRect.right - clientRect.left);

	if (availableWidth <= 0)
	{
		return;
	}

	auto parentPoint = pt;
	MapWindowPoints(GetHWND(), GetParent(GetHWND()), &parentPoint, 1);
	m_resizedCallback(
		static_cast<double>(parentPoint.x - m_contentBounds.left - m_dragOffset) / availableWidth);
}
