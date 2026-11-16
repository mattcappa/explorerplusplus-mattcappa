// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ShellBrowser/ColumnHelper.h"
#include "ResourceLoaderFake.h"
#include "ShellBrowser/FolderSettings.h"
#include <gtest/gtest.h>
#include <magic_enum/magic_enum.hpp>

TEST(ColumnHelperTest, ScalesHiddenAndVisibleColumnsForAllFolderTypes)
{
	FolderColumns columns;
	for (auto *set : { &columns.realFolderColumns, &columns.myComputerColumns,
			 &columns.controlPanelColumns, &columns.recycleBinColumns, &columns.printersColumns,
			 &columns.networkConnectionsColumns, &columns.myNetworkPlacesColumns })
	{
		*set = { { ColumnType::Name, TRUE, 240 }, { ColumnType::Type, FALSE, 180 } };
	}
	auto original = columns;
	ScaleColumnWidths(columns, 144, 96);
	for (const auto *set : { &columns.realFolderColumns, &columns.myComputerColumns,
			 &columns.controlPanelColumns, &columns.recycleBinColumns, &columns.printersColumns,
			 &columns.networkConnectionsColumns, &columns.myNetworkPlacesColumns })
	{
		EXPECT_EQ(set->at(0).width, 160);
		EXPECT_EQ(set->at(1).width, 120);
		EXPECT_TRUE(set->at(0).checked);
		EXPECT_FALSE(set->at(1).checked);
	}
	ScaleColumnWidths(columns, 96, 144);
	EXPECT_EQ(columns, original);
}

TEST(ColumnHelperTest, CheckEveryColumnHasAName)
{
	ResourceLoaderFake resourceLoader;

	// Each column should have a name. `GetColumnName` will trigger a CHECK failure if a particular
	// column isn't handled, so this test will fail in that case.
	for (auto columnType : magic_enum::enum_values<ColumnType>())
	{
		GetColumnName(&resourceLoader, columnType);
	}
}
