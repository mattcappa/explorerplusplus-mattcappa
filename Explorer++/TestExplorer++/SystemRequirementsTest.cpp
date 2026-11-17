// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "SystemRequirements.h"
#include <gtest/gtest.h>

TEST(SystemRequirementsTest, RejectsVersionsBeforeWindows11Version22H2)
{
	EXPECT_FALSE(SystemRequirements::IsSupportedWindowsVersion(6, 1, 7601));
	EXPECT_FALSE(SystemRequirements::IsSupportedWindowsVersion(6, 3, 9600));
	EXPECT_FALSE(SystemRequirements::IsSupportedWindowsVersion(10, 0, 19045));
	EXPECT_FALSE(SystemRequirements::IsSupportedWindowsVersion(10, 0, 22000));
	EXPECT_FALSE(SystemRequirements::IsSupportedWindowsVersion(10, 0, 22620));
}

TEST(SystemRequirementsTest, AcceptsMinimumBuildAndLaterBuilds)
{
	EXPECT_TRUE(SystemRequirements::IsSupportedWindowsVersion(10, 0, 22621));
	EXPECT_TRUE(SystemRequirements::IsSupportedWindowsVersion(10, 0, 22631));
	EXPECT_TRUE(SystemRequirements::IsSupportedWindowsVersion(10, 0, 26100));
}

TEST(SystemRequirementsTest, AcceptsNewerVersionFamiliesEvenWithLowerBuildNumbers)
{
	EXPECT_TRUE(SystemRequirements::IsSupportedWindowsVersion(10, 1, 0));
	EXPECT_TRUE(SystemRequirements::IsSupportedWindowsVersion(11, 0, 0));
}

TEST(SystemRequirementsTest, CheckAcceptsSupportedWindowsVersion)
{
	auto result = SystemRequirements::CheckWindowsVersion(
		[](OSVERSIONINFOW &version) -> HRESULT
		{
			version.dwMajorVersion = 10;
			version.dwMinorVersion = 0;
			version.dwBuildNumber = 22621;
			return S_OK;
		});
	EXPECT_HRESULT_SUCCEEDED(result);
}

TEST(SystemRequirementsTest, CheckReportsUnsupportedWindowsVersion)
{
	auto result = SystemRequirements::CheckWindowsVersion(
		[](OSVERSIONINFOW &version) -> HRESULT
		{
			version.dwMajorVersion = 10;
			version.dwMinorVersion = 0;
			version.dwBuildNumber = 22620;
			return S_OK;
		});
	EXPECT_EQ(result, HRESULT_FROM_WIN32(ERROR_OLD_WIN_VERSION));
}

TEST(SystemRequirementsTest, CheckPreservesQueryFailure)
{
	auto result = SystemRequirements::CheckWindowsVersion(
		[](OSVERSIONINFOW &version) -> HRESULT
		{
			version.dwMajorVersion = 10;
			version.dwMinorVersion = 0;
			version.dwBuildNumber = 26100;
			return E_ACCESSDENIED;
		});
	EXPECT_EQ(result, E_ACCESSDENIED);
}
