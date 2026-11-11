// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <glog/logging.h>
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <optional>

// Two tabs share the browser's controls; selecting a different tab replaces only the active side.
class SplitPaneState
{
public:
	void SelectTab(int tabId)
	{
		if (m_otherTabId == tabId)
		{
			m_otherTabId = m_selectedTabId;
			m_activeOnRight = !m_activeOnRight;
		}

		m_selectedTabId = tabId;
	}

	void Enable(int otherTabId)
	{
		CHECK(m_selectedTabId);
		CHECK_NE(*m_selectedTabId, otherTabId);
		m_otherTabId = otherTabId;
	}

	void Disable()
	{
		m_otherTabId.reset();
		m_activeOnRight = false;
	}

	bool IsEnabled() const
	{
		return m_otherTabId.has_value();
	}

	bool IsVisible(int tabId) const
	{
		return m_selectedTabId == tabId || m_otherTabId == tabId;
	}

	std::optional<int> GetOtherTabId() const
	{
		return m_otherTabId;
	}

	void SetSplitFraction(double fraction)
	{
		CHECK(std::isfinite(fraction));
		m_splitFraction = std::clamp(fraction, 0.1, 0.9);
	}

	RECT GetDividerBounds(const RECT &bounds, int dividerWidth) const
	{
		auto width = std::max(0L, bounds.right - bounds.left);
		auto gap = std::clamp(static_cast<LONG>(dividerWidth), 0L, width);
		auto leftWidth = static_cast<LONG>((width - gap) * m_splitFraction);
		return { bounds.left + leftWidth, bounds.top, bounds.left + leftWidth + gap,
			bounds.bottom };
	}

	RECT GetBounds(int tabId, const RECT &bounds, int dividerWidth) const
	{
		if (!IsEnabled())
		{
			return bounds;
		}

		auto divider = GetDividerBounds(bounds, dividerWidth);
		bool onRight = (m_selectedTabId == tabId) ? m_activeOnRight : !m_activeOnRight;

		if (onRight)
		{
			return { divider.right, bounds.top, bounds.right, bounds.bottom };
		}

		return { bounds.left, bounds.top, divider.left, bounds.bottom };
	}

private:
	std::optional<int> m_selectedTabId;
	std::optional<int> m_otherTabId;
	bool m_activeOnRight = false;
	double m_splitFraction = 0.5;
};
