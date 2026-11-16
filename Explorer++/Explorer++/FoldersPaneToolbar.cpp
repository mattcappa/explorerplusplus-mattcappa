// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "FoldersPaneToolbar.h"
#include "Config.h"
#include "FoldersPaneToolbarView.h"
#include "MainResource.h"
#include "ResourceLoader.h"
#include "../Helper/DpiCompatibility.h"

namespace
{

class CloseButton : public ToolbarButton
{
public:
	CloseButton(Config *config, const ResourceLoader *resourceLoader, int imageIndex) :
		ToolbarButton(std::bind(&CloseButton::OnClicked, this)),
		m_config(config),
		m_resourceLoader(resourceLoader),
		m_imageIndex(imageIndex)
	{
	}

	std::wstring GetText() const override
	{
		return {};
	}

	bool ShouldShowText() const override
	{
		return false;
	}

	std::wstring GetTooltipText() const override
	{
		return m_resourceLoader->LoadString(IDS_HIDE_FOLDERS_PANE);
	}

	std::optional<int> GetImageIndex() const override
	{
		return m_imageIndex;
	}

private:
	void OnClicked()
	{
		m_config->showFolders = false;
	}

	Config *const m_config;
	const ResourceLoader *const m_resourceLoader;
	const int m_imageIndex;
};

}

FoldersPaneToolbar::FoldersPaneToolbar(FoldersPaneToolbarView *view, Config *config,
	MenuHelpTextHost *menuHelpTextHost, const AcceleratorManager *acceleratorManager,
	const ResourceLoader *resourceLoader) :
	ToolbarBase(view, menuHelpTextHost, acceleratorManager),
	m_view(view),
	m_resourceLoader(resourceLoader)
{
	UpdateImageList();
	auto closeButton = std::make_unique<CloseButton>(config, resourceLoader, 0);
	view->AddButton(std::move(closeButton), 0);
	m_dpiConnection = view->dpiChangedSignal.AddObserver(
		std::bind_front(&FoldersPaneToolbar::UpdateImageList, this));
}

void FoldersPaneToolbar::UpdateImageList()
{
	auto &dpiCompat = DpiCompatibility::GetInstance();
	UINT dpi = dpiCompat.GetDpiForWindow(m_view->GetHWND());
	int iconWidth = dpiCompat.GetSystemMetricsForDpi(SM_CXSMICON, dpi);
	int iconHeight = dpiCompat.GetSystemMetricsForDpi(SM_CYSMICON, dpi);
	wil::unique_himagelist imageList(
		ImageList_Create(iconWidth, iconHeight, ILC_COLOR32 | ILC_MASK, 1, 0));
	CHECK(imageList);

	wil::unique_hbitmap closeIcon =
		m_resourceLoader->LoadBitmapFromPNGAndScale(Icon::CloseButton, iconWidth, iconHeight);
	int closeIconImageIndex = ImageList_Add(imageList.get(), closeIcon.get(), nullptr);
	CHECK_EQ(closeIconImageIndex, 0);
	m_view->SetImageList(imageList.get());
	m_imageList = std::move(imageList);
}
