// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ShellItemContextMenu.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "PidlTestHelper.h"
#include "RootMenuViewTestApi.h"
#include "ShellBrowser/ShellBrowser.h"
#include "TabBar.h"
#include "TestMenuView.h"
#include <gtest/gtest.h>

class ShellItemContextMenuTest : public BrowserTestBase
{
};

TEST_F(ShellItemContextMenuTest, OpenFolderInOtherPane)
{
	auto *browser = AddBrowser();
	auto *active = browser->AddTab(L"c:\\");
	auto directory = CreateSimplePidlForTest(L"c:\\");
	auto folder = CreateSimplePidlForTest(L"c:\\windows");
	TestMenuView menuView;
	ShellItemContextMenu menu(&menuView, &m_acceleratorManager, directory, { folder.GetLastItem() },
		browser->GetHWND(), nullptr, ShellItemContextMenu::Flags::None, browser, &m_resourceLoader);

	RootMenuViewTestApi::SelectItem(&menuView, ShellItemContextMenu::Item::OpenInOtherPane, false,
		false);

	auto *tabBar = browser->GetActiveTabBar();
	ASSERT_EQ(tabBar->GetNumTabs(), 2);
	EXPECT_TRUE(tabBar->IsTabSelected(*active));
	auto &other = tabBar->GetTab(*tabBar->GetSplitPaneState().GetOtherTabId());
	EXPECT_EQ(other.GetShellBrowser()->GetDirectory(), folder);
}
