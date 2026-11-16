// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/com.h>
#include <wil/resource.h>
#include <commoncontrols.h>
#include <map>
#include <utility>

// Owns window-sized copies of shell icons without changing the shared system image list.
class ShellIconImageList
{
public:
	explicit ShellIconImageList(UINT dpi);

	HIMAGELIST GetImageList() const;
	int GetIconIndex(int systemIconIndex, int overlayIndex = 0, bool refresh = false);

private:
	const int m_size;
	wil::com_ptr_nothrow<IImageList> m_systemImageList;
	wil::unique_himagelist m_imageList;
	std::map<std::pair<int, int>, int> m_iconIndexes;
};
