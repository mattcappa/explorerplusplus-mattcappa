---
name: explorerplusplus-win32-ui
description: Explorer++ Win32 UI patterns for dialogs, resources, menus, commands, theming and DPI. Use for visual changes or Win32 UI work.
---

# Win32 UI in Explorer++

Explorer++ uses Win32 controls and dialog templates directly (no XAML or MFC). Check Microsoft Learn for API behavior and message semantics rather than guessing.

## Resource files (read this first)

`Explorer++\Explorer++\Explorer++Main.rc` is the only compiled resource script and includes:

| File | Contents | Encoding |
| --- | --- | --- |
| `Explorer++.rc` | Dialogs, menus, string tables and accelerators (header: `resource.h`) | **UTF-16LE with a BOM, CRLF** |
| `Explorer++_NoTranslation.rc` | PNG icons and bitmaps for every icon set (header: `NoTranslationResource.h`) | 8-bit (ASCII/UTF-8, no BOM) |
| `Explorer++VersionInfo.rc` | `VERSIONINFO` | **UTF-16LE with a BOM** |

Include `MainResource.h` from C++ when resource IDs are needed. All resources are in the exe; do not reintroduce translation DLLs.

**Encoding matters:** a plain UTF-8 write corrupts UTF-16 resources. Use the Visual Studio resource editor or an encoding-aware tool:

```python
p = r'Explorer++\Explorer++\Explorer++.rc'
t = open(p, 'rb').read().decode('utf-16')          # BOM-aware
t = t.replace(old_text_with_crlf, new_text_with_crlf)  # use '\r\n' line endings
open(p, 'wb').write(t.encode('utf-16'))             # writes UTF-16LE + BOM
```

When changing IDs, update `resource.h`, preserve its `_APS_NEXT_*` values, and search all uses before deleting an ID.

## Strings

- Load UI text with `ResourceLoader::LoadString(IDS_...)`, not hard-coded C++ strings. `ResourceLoader` is available through `AppServices`; tests can use `ResourceLoaderFake`.
- Format string-table text with `fmt::format` or `std::format`.

## Icons

- `Icon` (`Icon.h`) and `IconMappings.h` map logical icons to resources per icon set. Do not renumber the sets. When adding an icon, update each set's PNG, resource entry/ID, enum and mapping.
- Load icons at the current DPI with `ResourceLoader::LoadIconFromPNGForDpi` or `LoadBitmapFromPNGForDpi`.

## Dialogs

- Derive new dialogs from `BaseDialog` and create them through `ResourceLoader` so automatic theming applies. Do not call `DialogBoxParam`/`CreateDialogParam` directly.
- Use `ShowModalDialog()` for modal UI. Register modeless dialogs with `ModelessDialogList`; override sizing and save-state hooks when applicable.
- Options-page wiring for persisted settings is in `explorerplusplus-config-settings`.

## Dark mode and theming

- Use `ThemeManager`'s current `ColorProvider` for custom-drawn colors and redraw on dark-mode changes. Track new top-level windows and theme dynamically created child controls.
- Test visual changes in both light and dark themes.

## Commands, menus and accelerators

- IDs are in `resource.h`; menus may be resource-defined or built by `MainMenu`. New commands generally go through `BrowserCommandController`; context-sensitive commands target the focused `BrowserCommandTarget`. Some older commands remain in `MsgHandler.cpp`.
- Default shortcuts are in `DefaultAccelerators.cpp`; `AcceleratorManager` holds the active table. Context menus use `MenuView`/`PopupMenuView` and Shell `IContextMenu` wrappers.

## DPI, fonts and layout

- The app is per-monitor DPI aware. Use `DpiCompatibility` to get window DPI and scale dimensions from 96 DPI; don't hard-code pixels.
- Respect the configurable main font. Use `ResizableDialogHelper`/`ResizableDialogControl` for dialog anchoring.

## Window plumbing helpers

- Prefer `WindowSubclass` over raw `SetWindowSubclass`, and `wil::unique_*` wrappers for handles.
- `MessageForwarder` is the dialog handler base; `EventWindow` is for message-only windows.
