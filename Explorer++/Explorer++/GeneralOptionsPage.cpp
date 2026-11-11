// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "GeneralOptionsPage.h"
#include "AppController.h"
#include "AppInfo.h"
#include "Config.h"
#include "MainResource.h"
#include "ResourceLoader.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/Helper.h"
#include "../Helper/ResizableDialogHelper.h"
#include "../Helper/WindowHelper.h"
#include <fmt/format.h>
#include <fmt/xchar.h>

using namespace DefaultFileManager;

const std::unordered_map<ReplaceExplorerMode, int> REPLACE_EXPLORER_ENUM_CONTROL_ID_MAPPINGS = {
	{ ReplaceExplorerMode::None, IDC_OPTION_REPLACEEXPLORER_NONE },
	{ ReplaceExplorerMode::FileSystem, IDC_OPTION_REPLACEEXPLORER_FILESYSTEM },
	{ ReplaceExplorerMode::All, IDC_OPTION_REPLACEEXPLORER_ALL }
};

GeneralOptionsPage::GeneralOptionsPage(HWND parent, const ResourceLoader *resourceLoader,
	AppController *appController, Config *config, SettingChangedCallback settingChangedCallback,
	HWND tooltipWindow) :
	OptionsPage(IDD_OPTIONS_GENERAL, IDS_OPTIONS_GENERAL_TITLE, parent, resourceLoader, config,
		settingChangedCallback, tooltipWindow),
	m_appController(appController)
{
}

std::unique_ptr<ResizableDialogHelper> GeneralOptionsPage::InitializeResizeDialogHelper()
{
	std::vector<ResizableDialogControl> controls;
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_GROUP_DEFAULT_FILE_MANAGER), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_OPTION_REPLACEEXPLORER_NONE),
		MovingType::None, SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_OPTION_REPLACEEXPLORER_FILESYSTEM),
		MovingType::None, SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_OPTION_REPLACEEXPLORER_ALL), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_GROUP_GENERAL_SETTINGS), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_OPTION_XML), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_STATIC_NEW_TAB_FOLDER), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_DEFAULT_NEWTABDIR_EDIT), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_DEFAULT_NEWTABDIR_BUTTON),
		MovingType::Horizontal, SizingType::None);
	return std::make_unique<ResizableDialogHelper>(GetDialog(), controls);
}

void GeneralOptionsPage::InitializeControls()
{
	CheckRadioButton(GetDialog(), IDC_OPTION_REPLACEEXPLORER_NONE, IDC_OPTION_REPLACEEXPLORER_ALL,
		REPLACE_EXPLORER_ENUM_CONTROL_ID_MAPPINGS.at(m_config->replaceExplorerMode));

	if (m_appController->GetSaveLocation() == SaveLocation::ConfigFile)
	{
		CheckDlgButton(GetDialog(), IDC_OPTION_XML, BST_CHECKED);
	}

	UINT dpi = DpiCompatibility::GetInstance().GetDpiForWindow(GetDialog());
	m_newTabDirectoryIcon = m_resourceLoader->LoadIconFromPNGForDpi(Icon::Folder, 16, 16, dpi);

	HWND hButton = GetDlgItem(GetDialog(), IDC_DEFAULT_NEWTABDIR_BUTTON);
	SendMessage(hButton, BM_SETIMAGE, IMAGE_ICON, (LPARAM) m_newTabDirectoryIcon.get());

	HWND hEdit = GetDlgItem(GetDialog(), IDC_DEFAULT_NEWTABDIR_EDIT);
	SetNewTabDirectory(hEdit, m_config->defaultTabDirectory.c_str());
}

void GeneralOptionsPage::SetNewTabDirectory(HWND hEdit, const TCHAR *szPath)
{
	unique_pidl_absolute pidl;
	HRESULT hr = SHParseDisplayName(szPath, nullptr, wil::out_param(pidl), 0, nullptr);

	if (SUCCEEDED(hr))
	{
		SetNewTabDirectory(hEdit, pidl.get());
	}
}

void GeneralOptionsPage::SetNewTabDirectory(HWND hEdit, PCIDLIST_ABSOLUTE pidl)
{
	/* Check if the specified folder is real or virtual. */
	SFGAOF attributes = SFGAO_FILESYSTEM;
	GetItemAttributes(pidl, &attributes);

	DWORD nameFlags;

	if (attributes & SFGAO_FILESYSTEM)
	{
		nameFlags = SHGDN_FORPARSING;
	}
	else
	{
		nameFlags = SHGDN_INFOLDER;
	}

	std::wstring newTabDir;
	GetDisplayName(pidl, nameFlags, newTabDir);

	SendMessage(hEdit, WM_SETTEXT, 0, (LPARAM) newTabDir.c_str());
}

void GeneralOptionsPage::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	if (HIWORD(wParam) != 0)
	{
		switch (HIWORD(wParam))
		{
		case EN_CHANGE:
		case CBN_SELCHANGE:
			m_settingChangedCallback();
			break;
		}
	}
	else
	{
		switch (LOWORD(wParam))
		{
		case IDC_OPTION_REPLACEEXPLORER_NONE:
		case IDC_OPTION_REPLACEEXPLORER_FILESYSTEM:
		case IDC_OPTION_REPLACEEXPLORER_ALL:
		case IDC_OPTION_XML:
			m_settingChangedCallback();
			break;

		case IDC_DEFAULT_NEWTABDIR_BUTTON:
			OnNewTabDirectoryButtonPressed();
			break;
		}
	}
}

void GeneralOptionsPage::OnNewTabDirectoryButtonPressed()
{
	std::wstring helperText = m_resourceLoader->LoadString(IDS_DEFAULTSETTINGS_NEWTAB);

	TCHAR szNewTabDir[MAX_PATH];
	GetDlgItemText(GetDialog(), IDC_DEFAULT_NEWTABDIR_EDIT, szNewTabDir, std::size(szNewTabDir));

	std::wstring virtualParsingPath;
	HRESULT hr = DecodeFriendlyPath(szNewTabDir, virtualParsingPath);

	if (SUCCEEDED(hr))
	{
		m_newTabDirectory = virtualParsingPath;
	}
	else
	{
		m_newTabDirectory = szNewTabDir;
	}

	TCHAR szDisplayName[MAX_PATH];
	BROWSEINFO browseInfo = {};
	browseInfo.hwndOwner = GetDialog();
	browseInfo.pidlRoot = nullptr;
	browseInfo.pszDisplayName = szDisplayName;
	browseInfo.lpszTitle = helperText.c_str();
	browseInfo.ulFlags = BIF_NEWDIALOGSTYLE;
	browseInfo.lpfn = BrowseFolderCallbackStub;
	browseInfo.lParam = reinterpret_cast<LPARAM>(this);
	unique_pidl_absolute pidl(SHBrowseForFolder(&browseInfo));

	if (pidl)
	{
		HWND editControl = GetDlgItem(GetDialog(), IDC_DEFAULT_NEWTABDIR_EDIT);
		SetNewTabDirectory(editControl, pidl.get());
	}
}

int CALLBACK GeneralOptionsPage::BrowseFolderCallbackStub(HWND hwnd, UINT msg, LPARAM lParam,
	LPARAM lpData)
{
	UNREFERENCED_PARAMETER(lParam);

	auto *page = reinterpret_cast<GeneralOptionsPage *>(lpData);
	page->BrowseFolderCallback(hwnd, msg, lParam);

	return 0;
}

void GeneralOptionsPage::BrowseFolderCallback(HWND hwnd, UINT msg, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch (msg)
	{
	case BFFM_INITIALIZED:
		SendMessage(hwnd, BFFM_SETSELECTION, TRUE,
			reinterpret_cast<LPARAM>(m_newTabDirectory.c_str()));
		break;
	}
}

void GeneralOptionsPage::SaveSettings()
{
	ReplaceExplorerMode replaceExplorerMode = ReplaceExplorerMode::None;

	if (IsDlgButtonChecked(GetDialog(), IDC_OPTION_REPLACEEXPLORER_NONE) == BST_CHECKED)
	{
		replaceExplorerMode = ReplaceExplorerMode::None;
	}
	else if (IsDlgButtonChecked(GetDialog(), IDC_OPTION_REPLACEEXPLORER_FILESYSTEM) == BST_CHECKED)
	{
		replaceExplorerMode = ReplaceExplorerMode::FileSystem;
	}
	else if (IsDlgButtonChecked(GetDialog(), IDC_OPTION_REPLACEEXPLORER_ALL) == BST_CHECKED)
	{
		replaceExplorerMode = ReplaceExplorerMode::All;
	}

	if (m_config->replaceExplorerMode != replaceExplorerMode)
	{
		OnReplaceExplorerSettingChanged(replaceExplorerMode);
	}

	bool savePreferencesToXmlFile =
		(IsDlgButtonChecked(GetDialog(), IDC_OPTION_XML) == BST_CHECKED);
	m_appController->SetSaveLocation(
		savePreferencesToXmlFile ? SaveLocation::ConfigFile : SaveLocation::Registry);

	HWND hEdit = GetDlgItem(GetDialog(), IDC_DEFAULT_NEWTABDIR_EDIT);
	std::wstring newTabDir = GetWindowString(hEdit);

	/* The folder may be virtual, in which case, it needs
	to be decoded. */
	std::wstring virtualParsingPath;
	HRESULT hr = DecodeFriendlyPath(newTabDir, virtualParsingPath);

	if (SUCCEEDED(hr))
	{
		m_config->defaultTabDirectory = virtualParsingPath;
	}
	else
	{
		m_config->defaultTabDirectory = newTabDir;
	}
}

void GeneralOptionsPage::OnReplaceExplorerSettingChanged(ReplaceExplorerMode updatedReplaceMode)
{
	bool settingChanged = UpdateReplaceExplorerSetting(updatedReplaceMode);

	if (!settingChanged)
	{
		// The default file manager setting was not changed, so reset the state of the file
		// manager radio buttons.
		CheckRadioButton(GetDialog(), IDC_OPTION_REPLACEEXPLORER_NONE,
			IDC_OPTION_REPLACEEXPLORER_ALL,
			REPLACE_EXPLORER_ENUM_CONTROL_ID_MAPPINGS.at(m_config->replaceExplorerMode));
	}
}

bool GeneralOptionsPage::UpdateReplaceExplorerSetting(ReplaceExplorerMode updatedReplaceMode)
{
	if (updatedReplaceMode != ReplaceExplorerMode::None
		&& m_config->replaceExplorerMode == ReplaceExplorerMode::None)
	{
		std::wstring warningMessage =
			m_resourceLoader->LoadString(IDS_OPTIONS_DIALOG_REPLACE_EXPLORER_WARNING);

		int selectedButton = MessageBox(GetDialog(), warningMessage.c_str(), AppInfo::NAME,
			MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2);

		if (selectedButton == IDNO)
		{
			return false;
		}
	}

	LSTATUS res = ERROR_SUCCESS;
	std::wstring menuText = m_resourceLoader->LoadString(IDS_OPEN_IN_EXPLORERPLUSPLUS);

	switch (updatedReplaceMode)
	{
	case ReplaceExplorerMode::None:
	{
		switch (m_config->replaceExplorerMode)
		{
		case ReplaceExplorerMode::FileSystem:
			res =
				RemoveAsDefaultFileManagerFileSystem(AppInfo::SHELL_DEFAULT_INTERNAL_COMMAND_NAME);
			break;

		case ReplaceExplorerMode::All:
			res = RemoveAsDefaultFileManagerAll(AppInfo::SHELL_DEFAULT_INTERNAL_COMMAND_NAME);
			break;

		case ReplaceExplorerMode::None:
			break;
		}
	}
	break;

	case ReplaceExplorerMode::FileSystem:
		RemoveAsDefaultFileManagerFileSystem(AppInfo::SHELL_DEFAULT_INTERNAL_COMMAND_NAME);
		RemoveAsDefaultFileManagerAll(AppInfo::SHELL_DEFAULT_INTERNAL_COMMAND_NAME);
		res = SetAsDefaultFileManagerFileSystem(AppInfo::SHELL_DEFAULT_INTERNAL_COMMAND_NAME,
			menuText.c_str());
		break;

	case ReplaceExplorerMode::All:
		RemoveAsDefaultFileManagerFileSystem(AppInfo::SHELL_DEFAULT_INTERNAL_COMMAND_NAME);
		RemoveAsDefaultFileManagerAll(AppInfo::SHELL_DEFAULT_INTERNAL_COMMAND_NAME);
		res = SetAsDefaultFileManagerAll(AppInfo::SHELL_DEFAULT_INTERNAL_COMMAND_NAME,
			menuText.c_str());
		break;
	}

	if (res == ERROR_SUCCESS)
	{
		m_config->replaceExplorerMode = updatedReplaceMode;

		return true;
	}
	else
	{
		auto systemErrorMessage = GetLastErrorMessage(res);
		std::wstring finalSystemErrorMessage;

		if (systemErrorMessage)
		{
			finalSystemErrorMessage = *systemErrorMessage;

			// Any trailing newlines are unnecessary, as they'll be added below when
			// appropriate.
			boost::trim(finalSystemErrorMessage);
		}
		else
		{
			std::wstring errorCodeTemplate = m_resourceLoader->LoadString(IDS_ERROR_CODE);
			finalSystemErrorMessage =
				fmt::format(fmt::runtime(errorCodeTemplate), fmt::arg(L"error_code", res));
		}

		std::wstring errorMessage = m_resourceLoader->LoadString(IDS_ERROR_REPLACE_EXPLORER_SETTING)
			+ L"\n\n" + finalSystemErrorMessage;

		MessageBox(GetDialog(), errorMessage.c_str(), AppInfo::NAME, MB_ICONWARNING);

		return false;
	}
}
