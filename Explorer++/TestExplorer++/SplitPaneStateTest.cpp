// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "SplitPaneState.h"
#include <gtest/gtest.h>

class SplitPaneStateTest : public testing::Test
{
protected:
	void SetUp() override
	{
		m_state.SelectTab(1);
	}

	void ExpectBounds(int tabId, const RECT &expected)
	{
		auto actual = m_state.GetBounds(tabId, m_bounds, 4);
		EXPECT_EQ(actual.left, expected.left);
		EXPECT_EQ(actual.top, expected.top);
		EXPECT_EQ(actual.right, expected.right);
		EXPECT_EQ(actual.bottom, expected.bottom);
	}

	SplitPaneState m_state;
	const RECT m_bounds = { 100, 40, 1104, 640 };
};

TEST_F(SplitPaneStateTest, SinglePane)
{
	EXPECT_FALSE(m_state.IsEnabled());
	EXPECT_TRUE(m_state.IsVisible(1));
	EXPECT_FALSE(m_state.IsVisible(2));
	ExpectBounds(1, m_bounds);
}

TEST_F(SplitPaneStateTest, Enable)
{
	m_state.Enable(2);
	EXPECT_TRUE(m_state.IsEnabled());
	EXPECT_TRUE(m_state.IsVisible(1));
	EXPECT_TRUE(m_state.IsVisible(2));
	EXPECT_FALSE(m_state.IsVisible(3));
	EXPECT_EQ(m_state.GetOtherTabId(), 2);
	ExpectBounds(1, { 100, 40, 600, 640 });
	ExpectBounds(2, { 604, 40, 1104, 640 });
}

TEST_F(SplitPaneStateTest, SwitchingSidesDoesNotMoveFolders)
{
	m_state.Enable(2);
	m_state.SelectTab(2);
	EXPECT_EQ(m_state.GetOtherTabId(), 1);
	ExpectBounds(1, { 100, 40, 600, 640 });
	ExpectBounds(2, { 604, 40, 1104, 640 });

	m_state.SelectTab(1);
	EXPECT_EQ(m_state.GetOtherTabId(), 2);
	ExpectBounds(1, { 100, 40, 600, 640 });
	ExpectBounds(2, { 604, 40, 1104, 640 });
}

TEST_F(SplitPaneStateTest, SelectingAnotherTabReplacesOnlyActiveSide)
{
	m_state.Enable(2);
	m_state.SelectTab(3);
	EXPECT_FALSE(m_state.IsVisible(1));
	EXPECT_TRUE(m_state.IsVisible(2));
	EXPECT_TRUE(m_state.IsVisible(3));
	ExpectBounds(3, { 100, 40, 600, 640 });
	ExpectBounds(2, { 604, 40, 1104, 640 });

	m_state.SelectTab(2);
	m_state.SelectTab(4);
	EXPECT_FALSE(m_state.IsVisible(2));
	EXPECT_TRUE(m_state.IsVisible(3));
	EXPECT_TRUE(m_state.IsVisible(4));
	ExpectBounds(3, { 100, 40, 600, 640 });
	ExpectBounds(4, { 604, 40, 1104, 640 });
}

TEST_F(SplitPaneStateTest, SelectingSameTabDoesNotChangeSides)
{
	m_state.Enable(2);
	m_state.SelectTab(2);
	m_state.SelectTab(2);
	EXPECT_EQ(m_state.GetOtherTabId(), 1);
	ExpectBounds(2, { 604, 40, 1104, 640 });
}

TEST_F(SplitPaneStateTest, DisableKeepsActiveTab)
{
	m_state.Enable(2);
	m_state.SelectTab(2);
	m_state.Disable();
	EXPECT_FALSE(m_state.IsEnabled());
	EXPECT_FALSE(m_state.GetOtherTabId());
	EXPECT_FALSE(m_state.IsVisible(1));
	EXPECT_TRUE(m_state.IsVisible(2));
	ExpectBounds(2, m_bounds);
}

TEST_F(SplitPaneStateTest, ReenableStartsWithActiveTabOnLeft)
{
	m_state.Enable(2);
	m_state.SelectTab(2);
	m_state.Disable();
	m_state.Enable(1);
	ExpectBounds(2, { 100, 40, 600, 640 });
	ExpectBounds(1, { 604, 40, 1104, 640 });
}

TEST_F(SplitPaneStateTest, OddWidthUsesAllAvailableSpace)
{
	m_state.Enable(2);
	RECT bounds = { 0, 0, 1005, 600 };
	auto left = m_state.GetBounds(1, bounds, 4);
	auto right = m_state.GetBounds(2, bounds, 4);
	EXPECT_EQ(left.right, 500);
	EXPECT_EQ(right.left, 504);
	EXPECT_EQ(right.right, 1005);
}

TEST_F(SplitPaneStateTest, SmallAndEmptyBoundsDoNotProduceNegativeWidths)
{
	m_state.Enable(2);

	for (LONG width : { 0L, 1L, 3L, 4L, 5L })
	{
		RECT bounds = { 10, 0, 10 + width, 600 };
		auto left = m_state.GetBounds(1, bounds, 4);
		auto right = m_state.GetBounds(2, bounds, 4);
		EXPECT_GE(left.right, left.left);
		EXPECT_GE(right.right, right.left);
		EXPECT_LE(right.left, bounds.right);
		EXPECT_EQ(right.right, bounds.right);
	}
}

TEST_F(SplitPaneStateTest, DividerScalesWithoutChangingOuterBounds)
{
	m_state.Enable(2);
	auto left = m_state.GetBounds(1, m_bounds, 8);
	auto right = m_state.GetBounds(2, m_bounds, 8);
	EXPECT_EQ(left.left, m_bounds.left);
	EXPECT_EQ(right.right, m_bounds.right);
	EXPECT_EQ(right.left - left.right, 8);
}

TEST_F(SplitPaneStateTest, DraggingDividerChangesWidths)
{
	m_state.Enable(2);
	m_state.SetSplitFraction(0.25);
	ExpectBounds(1, { 100, 40, 350, 640 });
	ExpectBounds(2, { 354, 40, 1104, 640 });
	auto divider = m_state.GetDividerBounds(m_bounds, 4);
	EXPECT_EQ(divider.left, 350);
	EXPECT_EQ(divider.right, 354);
}

TEST_F(SplitPaneStateTest, DividerIsClampedSoNeitherPaneCollapses)
{
	m_state.Enable(2);
	m_state.SetSplitFraction(-1);
	ExpectBounds(1, { 100, 40, 200, 640 });
	m_state.SetSplitFraction(2);
	ExpectBounds(1, { 100, 40, 1000, 640 });
}

TEST_F(SplitPaneStateTest, ReplacingOtherPanePreservesActiveSideAndDivider)
{
	m_state.Enable(2);
	m_state.SetSplitFraction(0.25);
	m_state.SelectTab(2);
	m_state.Enable(3);
	ExpectBounds(2, { 354, 40, 1104, 640 });
	ExpectBounds(3, { 100, 40, 350, 640 });
	EXPECT_FALSE(m_state.IsVisible(1));
}
