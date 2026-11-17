// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <windows.h>
#include <cstdint>
#include <tuple>

namespace SystemRequirements
{

using WindowsVersionQuery = HRESULT (*)(OSVERSIONINFOW &version);

constexpr bool IsSupportedWindowsVersion(std::uint32_t majorVersion, std::uint32_t minorVersion,
	std::uint32_t buildNumber)
{
	return std::tuple{ majorVersion, minorVersion, buildNumber } >= std::tuple{ 10u, 0u, 22621u };
}

HRESULT CheckWindowsVersion(WindowsVersionQuery query);

}
