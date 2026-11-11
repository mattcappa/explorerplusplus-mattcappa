// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <cstddef>
#include <optional>
#include <vector>

std::optional<std::vector<std::byte>> CopyResource(HINSTANCE resourceInstance, UINT resourceId,
	const WCHAR *type);
