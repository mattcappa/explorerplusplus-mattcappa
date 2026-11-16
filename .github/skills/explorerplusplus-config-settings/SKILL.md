---
name: explorerplusplus-config-settings
description: Checklist for adding, changing or removing a persisted Explorer++ setting across Config, storage, options, CLI and tests. Use when changing settings or their XML/registry storage.
---

# Settings and persistence in Explorer++

## Settings and persistence

- `Config` (`Explorer++\Explorer++\Config.h`) holds global settings and is owned by `App`. Prefer access through `AppServices::GetConfig()` or an explicit constructor dependency.
- Use a plain field for passive values. Use `ValueWrapper<T>` when UI must react live; `.set()` signals only on change. Store observer connections with the observing object. Copying `Config` copies values, not observers.
- `App::LoadSettings`/`SaveSettings` choose XML (portable mode) or registry storage. XML uses `config.xml` next to the exe, or `EXPLORERPP_CONFIG`; registry uses `HKCU\Software\Explorer++`. See `Storage.*`.
- Each persisted area has XML/registry storage classes behind `AppStorage`; low-level helpers are in `Explorer++\\Helper\\`. Settings save periodically and on exit.

## Adding a new setting (checklist)

Use an existing setting of the same type as a template. `showFullTitlePath` (a `ValueWrapper<bool>`) appears in every place below.

1. **`Config.h`**: add the member with a default value. Use `ValueWrapper<T>` if UI must react live.
2. **`ConfigXmlStorage.cpp`**:
   - Load it in `LoadFromNode`, using `GetBoolSetting`, `GetIntSetting` or similar with a PascalCase name such as `L"ShowFullTitlePath"`.
   - Save it in `SaveToNode` with `XMLSettings::AddAttributeToNode` / `EncodeBoolValue`.
   - The XML and registry names must match exactly.
3. **`ConfigRegistryStorage.cpp`**: load in `LoadFromKey` (`RegistrySettings::Read32BitValueFromRegistry`, `ReadString`, …) and save in `SaveToKey` (`RegistrySettings::SaveDword`, `SaveString`, …).
4. **Options UI** (if user-editable): add the control to the relevant `*OptionsPage`, load its current value, apply changes, and use the standard changed callback. See the `explorerplusplus-win32-ui` skill for dialog and resource rules.
5. **Consumers**: read the setting where it's used. For `ValueWrapper` settings, add an observer that refreshes the UI.
6. **Tests**:
   - Add a non-default value to `TestExplorer++\\ConfigStorageTestHelper.cpp` (`BuildReference()`); XML and registry round-trip tests then cover it.
   - For renamed or migrated settings, add a migration test using the resources in `TestExplorer++\\Resources\\`.
7. **Command-line override** (optional): add a CLI11 option in `CommandLine.cpp` (`app.add_option`/`add_flag`) and a field in `CommandLine::Settings` (`CommandLine.h`). Apply it after the config loads, as `App::App` does for `changeNotifyMode`. Add tests in `CommandLineTest.cpp`.
8. **Docs**: update `Documentation\User\History.txt` (65-character lines) and the relevant Sphinx page under `Documentation\User\Sphinx\menus\tools\options\`.

## Removing a setting

Remove it from `Config`, both storage backends, consumers, tests and (if present) the options UI/resources. Old XML and registry values are ignored unless they conflict with a replacement.
