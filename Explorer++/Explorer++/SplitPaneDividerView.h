// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ViewBase.h"
#include <functional>

class ThemeManager;

class SplitPaneDividerView : public ViewBase
{
public:
	SplitPaneDividerView(ViewBase *parent, const ThemeManager *themeManager);

	void SetResizedCallback(std::function<void(double)> callback);
	void SetBounds(const RECT &contentBounds, const RECT &dividerBounds, bool visible);

private:
	static HWND Create(ViewBase *parent);
	LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void Paint(HDC hdc);
	void OnMouseMove(const POINT &pt);

	const ThemeManager *const m_themeManager;
	std::function<void(double)> m_resizedCallback;
	RECT m_contentBounds = {};
	bool m_dragging = false;
	int m_dragOffset = 0;
};
