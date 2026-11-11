// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "SplitPaneDividerView.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include <gtest/gtest.h>

class SplitPaneDividerViewTest : public BrowserTestBase
{
protected:
	SplitPaneDividerViewTest() :
		m_browser(AddBrowser()),
		m_divider(m_browser->GetView(), &m_themeManager)
	{
		m_divider.SetBounds({ 0, 0, 1006, 600 }, { 500, 0, 506, 600 }, true);
	}

	BrowserWindowFake *const m_browser;
	SplitPaneDividerView m_divider;
};

TEST_F(SplitPaneDividerViewTest, DragReportsFractionWithoutInitialJump)
{
	double fraction = 0;
	m_divider.SetResizedCallback([&](double value) { fraction = value; });
	auto hwnd = m_divider.GetHWND();
	SendMessage(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(3, 10));
	SendMessage(hwnd, WM_MOUSEMOVE, MK_LBUTTON, MAKELPARAM(103, 10));
	EXPECT_DOUBLE_EQ(fraction, 0.6);
	SendMessage(hwnd, WM_LBUTTONUP, 0, MAKELPARAM(103, 10));
	EXPECT_NE(GetCapture(), hwnd);
}

TEST_F(SplitPaneDividerViewTest, DoubleClickResetsToEqualWidths)
{
	double fraction = 0;
	m_divider.SetResizedCallback([&](double value) { fraction = value; });
	SendMessage(m_divider.GetHWND(), WM_LBUTTONDBLCLK, MK_LBUTTON, MAKELPARAM(3, 10));
	EXPECT_DOUBLE_EQ(fraction, 0.5);
}

TEST_F(SplitPaneDividerViewTest, HidingDividerReleasesCapture)
{
	auto hwnd = m_divider.GetHWND();
	SendMessage(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(3, 10));
	ASSERT_EQ(GetCapture(), hwnd);
	m_divider.SetBounds({ 0, 0, 1006, 600 }, { 500, 0, 506, 600 }, false);
	EXPECT_NE(GetCapture(), hwnd);
}
