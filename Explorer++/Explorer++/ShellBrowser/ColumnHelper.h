// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Columns.h"
#include <string>

class ResourceLoader;
struct FolderColumns;

std::wstring GetColumnName(const ResourceLoader *resourceLoader, ColumnType columnType);
std::wstring GetColumnDescription(const ResourceLoader *resourceLoader, ColumnType columnType);
void ScaleColumnWidths(FolderColumns &columns, UINT oldDpi, UINT newDpi);
