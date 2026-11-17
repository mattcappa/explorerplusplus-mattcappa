// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/resource.h>
#include <windows.h>

class DpiCompatibility
{
public:
	static DpiCompatibility &GetInstance();

	BOOL WINAPI SystemParametersInfoForDpi(UINT uiAction, UINT uiParam, PVOID pvParam, UINT fWinIni,
		UINT dpi);
	int WINAPI GetSystemMetricsForDpi(int nIndex, UINT dpi);
	UINT WINAPI GetDpiForWindow(HWND hwnd);

	// Scales a value targeted at 96 DPI to the DPI of the specified window.
	int ScaleValue(HWND hwnd, int value);

	int PointsToPixelsForDefaultDpi(int pt);
	int PixelsToPointsForDefaultDpi(int px);
	int PointsToPixels(HWND hwnd, int pt);
	int PixelsToPoints(HWND hwnd, int px);

private:
	using SystemParametersInfoForDpiType = BOOL(
		WINAPI *)(UINT uiAction, UINT uiParam, PVOID pvParam, UINT fWinIni, UINT dpi);
	using GetSystemMetricsForDpiType = int(WINAPI *)(int nIndex, UINT dpi);
	using GetDpiForWindowType = UINT(WINAPI *)(HWND hwnd);

	DpiCompatibility();

	wil::unique_hmodule m_user32;

	SystemParametersInfoForDpiType m_SystemParametersInfoForDpi;
	GetSystemMetricsForDpiType m_GetSystemMetricsForDpi;
	GetDpiForWindowType m_GetDpiForWindow;

	int PointsToPixelsForDpi(int pt, UINT dpi);
	int PixelsToPointsForDpi(int px, UINT dpi);
};
