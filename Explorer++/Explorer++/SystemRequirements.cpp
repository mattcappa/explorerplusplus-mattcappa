// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "SystemRequirements.h"
#include <wil/result.h>

HRESULT SystemRequirements::CheckWindowsVersion(WindowsVersionQuery query)
{
	RETURN_HR_IF(E_INVALIDARG, !query);

	OSVERSIONINFOW version = {};
	RETURN_IF_FAILED(query(version));
	RETURN_HR_IF(HRESULT_FROM_WIN32(ERROR_OLD_WIN_VERSION),
		!IsSupportedWindowsVersion(version.dwMajorVersion, version.dwMinorVersion,
			version.dwBuildNumber));
	return S_OK;
}
