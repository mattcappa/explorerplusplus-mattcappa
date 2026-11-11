// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ShellBackgroundContextMenu.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "PidlTestHelper.h"
#include "RootMenuViewTestApi.h"
#include "ShellBrowser/ShellBrowser.h"
#include "TabBar.h"
#include "TestMenuView.h"
#include <gtest/gtest.h>

class ShellBackgroundContextMenuTest : public BrowserTestBase
{
};

TEST_F(ShellBackgroundContextMenuTest, OpenCurrentFolderInOtherPane)
{
	auto *browser = AddBrowser();
	auto *active = browser->AddTab(L"c:\\");
	auto directory = active->GetShellBrowser()->GetDirectory();
	TestMenuView menuView;
	ShellBackgroundContextMenu menu(&menuView, &m_acceleratorManager, directory, browser->GetHWND(),
		nullptr, ShellBackgroundContextMenu::Flags::None, browser,
		m_platformContext.GetClipboardStore(), &m_resourceLoader);

	RootMenuViewTestApi::SelectItem(&menuView, ShellBackgroundContextMenu::Item::OpenInOtherPane,
		false, false);

	auto *tabBar = browser->GetActiveTabBar();
	ASSERT_EQ(tabBar->GetNumTabs(), 2);
	EXPECT_TRUE(tabBar->IsTabSelected(*active));
	auto &other = tabBar->GetTab(*tabBar->GetSplitPaneState().GetOtherTabId());
	EXPECT_EQ(other.GetShellBrowser()->GetDirectory(), directory);
}
