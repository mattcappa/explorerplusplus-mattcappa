---
name: explorerplusplus-config-settings
description: Step-by-step guide to adding, changing or removing a persisted Explorer++ setting (Config struct, ValueWrapper observers, XML config file, registry, Options dialog page, command-line override, tests). Use whenever a task touches Config.h, ConfigXmlStorage, ConfigRegistryStorage, an *OptionsPage, or any *XmlStorage / *RegistryStorage class.
---

# Settings and persistence in Explorer++

## Where settings live

- **`Config`** (`Explorer++\Explorer++\Config.h`) is a single struct holding the global settings. It has a defaulted `operator==`, which the round-trip tests rely on. `App` owns the instance, and other code reaches it through `AppServices::GetConfig()` or a `Config *` constructor parameter.
- Settings come in two kinds:
  - **Plain fields** such as `bool`, `int` and enums. Read and write them directly.
  - **`ValueWrapper<T>`** (`ValueWrapper.h`) for settings that must update live UI. Read with `.get()` and write with `.set(value)`. `set()` fires the signal only when the value changes. Subscribe with `.addObserver(callback)` and store the connection, as `BrowserView.cpp` does with `showFullTitlePath`. Copying a `Config` copies values, not observers.
- **Storage backends.** `App::LoadSettings`/`SaveSettings` pick one:
  - **XML config file** (portable mode): `config.xml` next to the exe, or the path in the `EXPLORERPP_CONFIG` environment variable. The root node is `ExplorerPlusPlus` and settings are under `Settings`. See `Storage.h`/`Storage.cpp`.
  - **Registry**: `HKCU\Software\Explorer++` (`Storage::REGISTRY_APPLICATION_KEY_PATH`).
  - If a config file is present and readable, it wins. The user switches between the two with Tools > Options > General > "Run in portable mode" (`IDC_OPTION_XML`, which calls `AppController::SetSaveLocation`).
- `AppStorage` (`AppStorage.h`) is the interface, with `XmlAppStorage` and `RegistryAppStorage` implementing it. Each area has a pair of storage classes: `ConfigXmlStorage`/`ConfigRegistryStorage`, `TabXmlStorage`/`TabRegistryStorage`, `WindowXmlStorage`/…, `BookmarkXmlStorage`/…, `ColorRuleXmlStorage`/…, `ColumnXmlStorage`/…, `ApplicationToolbarXmlStorage`/…, `FrequentLocationsXmlStorage`/…, `RebarXmlStorage`/…, `StartupFoldersXmlStorage`/…, `MainToolbarStorage` and `CustomFontStorage`. Low-level helpers live in `Explorer++\Helper` (`XMLSettings`, `RegistrySettings`).
- Settings are saved every 30 seconds and on exit.

## Adding a new setting (checklist)

Use an existing setting of the same type as a template. `showFullTitlePath` (a `ValueWrapper<bool>`) appears in every place below.

1. **`Config.h`**: add the member with a default value. Use `ValueWrapper<T>` if UI must react live.
2. **`ConfigXmlStorage.cpp`**:
   - Load it in `LoadFromNode`, using `GetBoolSetting`, `GetIntSetting` or similar with a PascalCase name such as `L"ShowFullTitlePath"`.
   - Save it in `SaveToNode` with `XMLSettings::AddAttributeToNode` / `EncodeBoolValue`.
   - The XML and registry names must match exactly.
3. **`ConfigRegistryStorage.cpp`**: load in `LoadFromKey` (`RegistrySettings::Read32BitValueFromRegistry`, `ReadString`, …) and save in `SaveToKey` (`RegistrySettings::SaveDword`, `SaveString`, …).
4. **Options UI** (if the setting is user-editable). See the checklist below.
5. **Consumers**: read the setting where it's used. For `ValueWrapper` settings, add an observer that refreshes the UI.
6. **Tests**:
   - Set a non-default value in `TestExplorer++\ConfigStorageTestHelper.cpp` (`BuildReference()`). `ConfigXmlStorageTest.SaveLoad` and `ConfigRegistryStorageTest.SaveLoad` then cover the round trip automatically.
   - If you rename or migrate a setting, add a migration test with resource files in `TestExplorer++\Resources\` (see `config-migration.xml`/`.reg` and `ConfigXmlStorageTest.OpenZipFilesSettingMigration`).
7. **Command-line override** (optional): add a CLI11 option in `CommandLine.cpp` (`app.add_option`/`add_flag`) and a field in `CommandLine::Settings` (`CommandLine.h`). Apply it after the config loads, as `App::App` does for `changeNotifyMode`. Add tests in `CommandLineTest.cpp`.
8. **Docs**: update `Documentation\User\History.txt` (65-character lines) and the relevant Sphinx page under `Documentation\User\Sphinx\menus\tools\options\`.

## Options dialog pages

- `OptionsDialog.cpp` creates the pages in a fixed order with `AddPage(std::make_unique<XxxOptionsPage>(...))`: General, Startup, Appearance, Fonts, Files & Folders, Window, Tabs, Default Settings, Advanced.
- Each page derives from `OptionsPage` (`OptionsPage.h`) and implements:
  - `InitializeControls()`: populate controls from `m_config`.
  - `InitializeResizeDialogHelper()`: return a `ResizableDialogHelper` with one `ResizableDialogControl` entry per control that should move or stretch.
  - `OnCommand`/`OnNotify`: call `m_settingChangedCallback()` when a control changes, which enables the dialog's Apply button.
  - `SaveSettings()`: write the values back to `m_config`.
- The page layout is a `DIALOGEX` such as `IDD_OPTIONS_WINDOW` in `Explorer++.rc`, and the control IDs are `IDC_OPTION_*` / `IDC_SETTINGS_*` in `resource.h`. Remember that `Explorer++.rc` is **UTF-16LE with a BOM**; see the `explorerplusplus-win32-ui` skill.
- The Options search box uses the control text (`DoesPageContainText`), so give new controls descriptive labels.

## Removing a setting

Delete it from all of the places listed above, including the `BuildReference()` test helper, the dialog control, its `resource.h` IDs and any `ResizableDialogControl` entries. Old XML and registry values left on technicians' machines are ignored, so no clean-up code is needed unless the old value would conflict.
