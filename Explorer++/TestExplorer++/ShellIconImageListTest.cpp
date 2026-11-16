// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ShellIconImageList.h"
#include "../Helper/ShellHelper.h"
#include <gtest/gtest.h>

TEST(ShellIconImageListTest, ScalesIconsAndKeepsSystemIndexesSeparate)
{
	int folderIcon;
	ASSERT_HRESULT_SUCCEEDED(GetDefaultFolderIconIndex(folderIcon));
	int fileIcon;
	ASSERT_HRESULT_SUCCEEDED(GetDefaultFileIconIndex(fileIcon));

	for (UINT dpi : { 96, 120, 144, 192, 288, 96 })
	{
		ShellIconImageList imageList(dpi);
		int folderIndex = imageList.GetIconIndex(folderIcon);
		int fileIndex = imageList.GetIconIndex(fileIcon);
		EXPECT_NE(folderIndex, fileIndex);
		EXPECT_EQ(imageList.GetIconIndex(folderIcon), folderIndex);
		EXPECT_EQ(ImageList_GetImageCount(imageList.GetImageList()), 2);
		EXPECT_EQ(imageList.GetIconIndex(folderIcon, 0, true), folderIndex);
		EXPECT_EQ(ImageList_GetImageCount(imageList.GetImageList()), 2);
		int overlayIndex = imageList.GetIconIndex(folderIcon, 1);
		EXPECT_NE(overlayIndex, folderIndex);
		EXPECT_EQ(imageList.GetIconIndex(folderIcon, 1, true), overlayIndex);
		EXPECT_EQ(ImageList_GetImageCount(imageList.GetImageList()), 3);

		int width;
		int height;
		ASSERT_TRUE(ImageList_GetIconSize(imageList.GetImageList(), &width, &height));
		EXPECT_EQ(width, MulDiv(16, dpi, USER_DEFAULT_SCREEN_DPI));
		EXPECT_EQ(height, width);

		wil::unique_hicon icon(
			ImageList_GetIcon(imageList.GetImageList(), folderIndex, ILD_NORMAL));
		ASSERT_TRUE(icon);
		ICONINFO info = {};
		ASSERT_TRUE(GetIconInfo(icon.get(), &info));
		wil::unique_hbitmap color(info.hbmColor);
		wil::unique_hbitmap mask(info.hbmMask);
		BITMAP bitmap = {};
		ASSERT_NE(GetObject(color.get(), sizeof(bitmap), &bitmap), 0);
		EXPECT_EQ(bitmap.bmWidth, width);
		EXPECT_EQ(bitmap.bmHeight, height);
	}
}
