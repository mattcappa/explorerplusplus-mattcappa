// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellIconImageList.h"

ShellIconImageList::ShellIconImageList(UINT dpi) :
	m_size(MulDiv(16, dpi, USER_DEFAULT_SCREEN_DPI)),
	m_imageList(ImageList_Create(m_size, m_size, ILC_COLOR32 | ILC_MASK, 0, 10))
{
	CHECK(m_imageList);
	FAIL_FAST_IF_FAILED(SHGetImageList(SHIL_EXTRALARGE, IID_PPV_ARGS(&m_systemImageList)));
}

HIMAGELIST ShellIconImageList::GetImageList() const
{
	return m_imageList.get();
}

int ShellIconImageList::GetIconIndex(int systemIconIndex, int overlayIndex, bool refresh)
{
	auto key = std::pair(systemIconIndex, overlayIndex);
	auto itr = m_iconIndexes.find(key);
	if (itr != m_iconIndexes.end() && !refresh)
	{
		return itr->second;
	}

	wil::unique_hicon icon;
	FAIL_FAST_IF_FAILED(m_systemImageList->GetIcon(systemIconIndex,
		ILD_TRANSPARENT | INDEXTOOVERLAYMASK(overlayIndex), &icon));
	wil::unique_hicon scaledIcon(
		static_cast<HICON>(CopyImage(icon.get(), IMAGE_ICON, m_size, m_size, 0)));
	CHECK(scaledIcon);
	int index = ImageList_ReplaceIcon(m_imageList.get(),
		itr == m_iconIndexes.end() ? -1 : itr->second, scaledIcon.get());
	CHECK_NE(index, -1);
	m_iconIndexes.insert_or_assign(key, index);
	return index;
}
