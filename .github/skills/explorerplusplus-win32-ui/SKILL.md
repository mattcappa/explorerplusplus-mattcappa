---
name: explorerplusplus-win32-ui
description: Patterns for Explorer++ Win32 UI work, covering dialogs (BaseDialog, ResourceLoader), .rc resource files (UTF-16LE encoding), string and icon resources, menus, commands and accelerators, dark mode and theming, DPI scaling, and window subclassing. Use when adding or modifying dialogs, menu items, toolbar buttons, keyboard shortcuts, icons, strings or anything visual.
---

# Win32 UI in Explorer++

Explorer++ calls the Win32 API directly: common controls v6, a rebar, toolbars, a list view, a tree view and dialog templates. There's no XAML or MFC. Look up control behaviour and messages with the Microsoft Learn MCP server. The upstream "Useful references" are catch22.net for custom controls and *The Old New Thing* for Win32 edge cases.

## Resource files (read this first)

`Explorer++\Explorer++\Explorer++Main.rc` is the only `.rc` file compiled. It `#include`s the following files, which are each excluded from the build on their own:

| File | Contents | Encoding |
| --- | --- | --- |
| `Explorer++.rc` | Dialogs, menus, string tables and accelerators (header: `resource.h`) | **UTF-16LE with a BOM, CRLF** |
| `Explorer++_NoTranslation.rc` | PNG icons and bitmaps for every icon set (header: `NoTranslationResource.h`) | 8-bit (ASCII/UTF-8, no BOM) |
| `Explorer++VersionInfo.rc` | `VERSIONINFO` | **UTF-16LE with a BOM** |

`MainResource.h` includes both `resource.h` and `NoTranslationResource.h`; include it from C++ code. The "NoTranslation" name dates from when `Explorer++.rc` was shared with translation DLLs. This fork is English only, so every resource is compiled into the exe.

**Editing a UTF-16 `.rc` safely:** a plain UTF-8 write corrupts it. Use the Visual Studio resource editor, or Python:

```python
p = r'Explorer++\Explorer++\Explorer++.rc'
t = open(p, 'rb').read().decode('utf-16')          # BOM-aware
t = t.replace(old_text_with_crlf, new_text_with_crlf)  # use '\r\n' line endings
open(p, 'wb').write(t.encode('utf-16'))             # writes UTF-16LE + BOM
```

When you add or remove IDs:

- Update `resource.h` and keep its `_APS_NEXT_*` values valid.
- Grep for every use of an ID before you delete it. Some, such as `IDC_STATIC_RESTART_NOTICE`, are shared between pages.

## Strings

- Load user-visible text with `ResourceLoader::LoadString(IDS_...)`. Don't hard-code English in C++.
- `ResourceLoader` lives in `AppServices` and is passed to most UI classes. Tests use the real `Win32ResourceLoader`, or `TestExplorer++\ResourceLoaderFake.h`.
- Format strings with `fmt::format` or `std::format`, with the format string taken from the string table.

## Icons

- `Icon` (`Icon.h`) is the logical icon enum. `IconMappings.h` maps each `Icon` to a PNG resource for every `IconSet`. The sets are `Color = 0`, `Windows10 = 1` and `FluentUi = 2`, stored as integers. Don't renumber them. Resource IDs and mapping tables use matching suffixes (`_WINDOWS_10`, `_FLUENT_UI`), e.g. `ICON_RESOURCE_MAPPINGS_WINDOWS_10`.
- Load icons for the current DPI with `ResourceLoader::LoadIconFromPNGForDpi(icon, w, h, dpi)` or `LoadBitmapFromPNGForDpi`.
- To add an icon:
  1. Add the PNG to `res\` for each icon set.
  2. Add an entry to `Explorer++_NoTranslation.rc` and an ID to `NoTranslationResource.h`.
  3. Add an `Icon` enum value.
  4. Add the mapping in `IconMappings.h`.

## Dialogs

- **Modern pattern.** Derive from `BaseDialog` (`BaseDialog.h`), passing `resourceLoader`, the dialog ID and the parent to the constructor. Then:
  - Override `OnInitDialog`, `OnCommand`, `OnClose` and the other handlers (inherited from `MessageForwarder`).
  - Override `GetResizableControls()` and pass a `DialogSizingType` if the dialog is resizable.
  - Override `SaveState()` to persist size and position, using `DialogHelper`/`DialogStorageHelper`.
  - Show the dialog with `ShowModalDialog()`, or with `ShowModelessDialog(onDestroyed)` and register it in `ModelessDialogList` so that `App::IsModelessDialogMessage` routes keyboard input to it.
- **Theming is automatic.** Every dialog is created through `ResourceLoader::CreateModalDialog`/`CreateModelessDialog`. `Win32ResourceLoader` wraps the dialog proc in `ThemedDialogWrapper`, which attaches a `ThemeWindowTracker` on `WM_INITDIALOG`. Never call `DialogBoxParam`/`CreateDialogParam` directly, or the dialog won't get dark mode.
- **Options pages:** see the `explorerplusplus-config-settings` skill.

## Dark mode and theming

- `Config::theme` is a `ValueWrapper<Theme>` with the values Light, Dark and System. `DarkModeManager` (interface) and `DarkModeManagerImpl` decide whether dark mode is active and raise `AddDarkModeStatusChangedObserver`.
- `ThemeManager` supplies the `ColorProvider` (`LightModeColorProvider`/`DarkModeColorProvider`) for custom-drawn colours.
- `WindowThemeManager` (`WindowThemeManagerImpl.cpp`) themes standard controls by subclassing them. It handles the main window, dialogs, list views, tree views, toolbars, rebars, edits, buttons, combo boxes, the status bar, scrollbars, group boxes and tab controls. For a new top-level window, call `TrackTopLevelWindow(hwnd)`, or create a `ThemeWindowTracker`. After creating child controls dynamically, call `ApplyThemeToWindowAndChildren`.
- Custom-drawn controls must take their colours from `ThemeManager::GetCurrentColorProvider()` and redraw when the dark-mode status changes.
- Test every visual change in both themes.

## Commands, menus and accelerators

- Command IDs (`IDM_*` for menus, `IDA_*` for accelerator-only commands) are in `resource.h`. Menus are `MENU` resources in `Explorer++.rc`, and some are built dynamically by `MainMenu` (`MenuBase`).
- Execution path:
  1. A menu item, toolbar button or accelerator produces a command ID.
  2. `BrowserCommandController::ExecuteCommand(command, disposition)` runs it. `IsCommandEnabled` controls the enabled state.
  3. Context-sensitive commands such as copy, paste, rename and delete are forwarded to the focused `BrowserCommandTarget` (list view, tree view, address bar…) through `BrowserCommandTargetManager`.
  4. Some legacy commands are still handled in `Explorerplusplus` (`MsgHandler.cpp`).
- Keyboard shortcuts:
  - Defaults are in `DefaultAccelerators.cpp` (`g_defaultAccelerators`, an `ACCEL` array).
  - `AcceleratorManager` holds the live table. `App::MaybeTranslateAccelerator` translates it in the message loop.
  - Plugin accelerators use the `ACCELERATOR_PLUGIN_START_ID`…`END_ID` range (`MenuRanges.h`).
- Context menus use the `MenuView`/`PopupMenuView` and `ShellContextMenu` classes. Shell item menus delegate to `IContextMenu` (`ShellItemContextMenu`, `ShellBackgroundContextMenu`).

## DPI, fonts and layout

- The app is per-monitor DPI aware (`PerMonitorV2` in `Explorer++.exe.manifest`). Use `DpiCompatibility` (`Explorer++\Helper\DpiCompatibility.h`) to get a window's DPI and scale sizes. Never hard-code pixel sizes; scale them from 96 DPI.
- The main font is configurable (`Config::mainFont`, `MainFontSetter`), so controls should pick it up rather than assume the system font.
- `ResizableDialogHelper` / `ResizableDialogControl` handle dialog control anchoring.

## Window plumbing helpers

- `WindowSubclass` (Helper) gives RAII subclassing with `std::function` callbacks. Prefer it to raw `SetWindowSubclass`.
- `MessageForwarder` is the virtual-handler base for dialogs. `EventWindow` provides message-only windows for system notifications.
- Use `wil::unique_*` RAII wrappers for every handle (`unique_hicon`, `unique_hbitmap`, `unique_hmenu`, `unique_hwnd`…).
- Fail fast with `CHECK(...)` and `DCHECK(...)` (glog) on programming errors.
