// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ResourceHelper.h"
#include <cstring>

// Note that the type parameter isn't necessarily a string. For example, it may be set to RT_DIALOG,
// which is simply an integer with a cast. So, it's not valid to change that parameter to a const
// std::wstring&.
std::optional<std::vector<std::byte>> CopyResource(HINSTANCE resourceInstance, UINT resourceId,
	const WCHAR *type)
{
	HRSRC resourceInformationHandle =
		FindResource(resourceInstance, MAKEINTRESOURCE(resourceId), type);

	if (!resourceInformationHandle)
	{
		return std::nullopt;
	}

	DWORD resourceSize = SizeofResource(resourceInstance, resourceInformationHandle);

	if (resourceSize == 0)
	{
		return std::nullopt;
	}

	HGLOBAL resourceHandle = LoadResource(resourceInstance, resourceInformationHandle);

	if (!resourceHandle)
	{
		return std::nullopt;
	}

	const void *resourceData = LockResource(resourceHandle);

	if (!resourceData)
	{
		return std::nullopt;
	}

	std::vector<std::byte> copiedData(resourceSize);
	std::memcpy(copiedData.data(), resourceData, resourceSize);

	return copiedData;
}
