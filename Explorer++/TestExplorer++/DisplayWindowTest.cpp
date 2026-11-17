// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "DisplayWindow/DisplayWindow.h"
#include "BrowserTestBase.h"
#include "TestParentView.h"
#include "../Helper/DpiCompatibility.h"
#include <gtest/gtest.h>
#include <wil/resource.h>

class DisplayWindowTest : public BrowserTestBase
{
protected:
	DisplayWindowTest() :
		m_displayWindow(DisplayWindow::Create(m_parent.GetHWND(), DisplayWindow::Mode::Preview,
			nullptr, &m_appServices, &m_config)),
		m_ownedWindow(m_displayWindow->GetHWND())
	{
	}

	void SetDpi(UINT dpi)
	{
		m_displayWindow->m_dpi = dpi;
		m_displayWindow->OnFontConfigChanged();
	}

	void Resize(int width, int height)
	{
		ASSERT_TRUE(SetWindowPos(m_ownedWindow.get(), nullptr, 0, 0, width, height,
			SWP_NOZORDER | SWP_NOACTIVATE));
	}

	std::vector<RECT> GetTextRects()
	{
		auto dc = wil::GetDC(m_ownedWindow.get());
		return m_displayWindow->CalculateTextRects(dc.get());
	}

	RECT GetIconRect() const
	{
		return m_displayWindow->GetIconRect();
	}

	LOGFONT GetFont() const
	{
		LOGFONT font;
		constexpr int fontSize = sizeof(font);
		EXPECT_EQ(GetObject(m_displayWindow->m_font.get(), fontSize, &font), fontSize);
		return font;
	}

	TestParentView m_parent;
	DisplayWindow *m_displayWindow;
	wil::unique_hwnd m_ownedWindow;
};

TEST_F(DisplayWindowTest, TextDoesNotOverlapIconAtDifferentDpis)
{
	DisplayWindow_BufferText(m_ownedWindow.get(), L"Selected folder");
	DisplayWindow_BufferText(m_ownedWindow.get(), L"File folder");

	for (UINT dpi : { 96, 120, 144, 192, 288, 144, 96 })
	{
		SCOPED_TRACE(dpi);
		SetDpi(dpi);
		Resize(MulDiv(800, dpi, 96), MulDiv(90, dpi, 96));
		auto rects = GetTextRects();
		ASSERT_EQ(rects.size(), 2u);
		auto iconRect = GetIconRect();
		EXPECT_GE(rects[0].left - iconRect.right, MulDiv(12, dpi, 96));
		EXPECT_EQ(rects[0].left, rects[1].left);
		EXPECT_EQ(GetFont().lfHeight, MulDiv(m_config.displayWindowFont.get().lfHeight, dpi, 96));
	}
}

TEST_F(DisplayWindowTest, LargeFontMovesOverflowingRowsToNewColumns)
{
	auto font = m_config.displayWindowFont.get();
	font.lfHeight = -35;
	m_config.displayWindowFont = font;
	SetDpi(96);
	int height = m_displayWindow->GetMinimumHeight();
	Resize(1200, height);

	for (const auto *text : { L"First", L"Second", L"Third", L"Fourth" })
	{
		DisplayWindow_BufferText(m_ownedWindow.get(), text);
	}

	auto rects = GetTextRects();
	ASSERT_EQ(rects.size(), 4u);
	for (size_t i = 0; i < rects.size(); i++)
	{
		EXPECT_EQ(rects[i].top, 20);
		EXPECT_LE(rects[i].bottom, height - 10);
		if (i > 0)
		{
			EXPECT_GE(rects[i].left - rects[i - 1].right, 50);
		}
	}
}

TEST_F(DisplayWindowTest, DpiNotificationRefreshesConfiguredFont)
{
	SetDpi(288);
	SendMessage(m_ownedWindow.get(), WM_DPICHANGED_AFTERPARENT, 0, 0);
	auto dpi = DpiCompatibility::GetInstance().GetDpiForWindow(m_ownedWindow.get());
	EXPECT_EQ(GetFont().lfHeight, MulDiv(m_config.displayWindowFont.get().lfHeight, dpi, 96));

	auto font = m_config.displayWindowFont.get();
	font.lfHeight = -20;
	m_config.displayWindowFont = font;
	EXPECT_EQ(GetFont().lfHeight, MulDiv(-20, dpi, 96));
}
