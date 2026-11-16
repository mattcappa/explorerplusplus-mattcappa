# AGENTS.md: Explorer++ (internal IT technician build)

This is an internal fork of [Explorer++](https://github.com/derceg/explorerplusplus), a Win32/C++20 file manager, used only by IT technicians. This file is the **single source** of always-loaded AI guidance. `.github/copilot-instructions.md` and `CLAUDE.md` only point here, so edit this file and keep it short.

## Product rules
- **English only.** Don't reintroduce translation DLLs, the `Language` option or `--language` switch, `LAYOUT_RTL` handling, or per-language resources. OS-driven formatting of dates, numbers and sizes stays.
- **Windows 11 is the eventual target.** Keep compatibility code for older Windows unless asked to remove it. New code may use Windows 10+ APIs only where the project already does.
- Upstream conventions in [CONTRIBUTING.md](CONTRIBUTING.md) apply.

## Stay on track
- Make surgical changes that do only what was asked. Don't reformat, rename or "tidy" untouched code. Ask before widening scope.
- The working tree often holds the user's uncommitted work. Never revert, stage or overwrite files you weren't asked to change.
- Follow the newer pattern used by the nearest component (`AppServices`, interfaces, `*Model`/`*View`, signals), not the legacy `Explorerplusplus`/`*Handler.cpp` style.

## Save tokens
- Never search or read `vcpkg_installed/`, `**/x64/`, `Explorer++/ThirdParty/`, `.vs/`, or `*.vcxproj.filters` (except to add an entry).
- Grep first, then `view` with a line range. These files are huge: `Explorer++.rc` (~180 KB), `Explorer++.vcxproj` (~130 KB), `resource.h` (~50 KB), `ListView.cpp`, `ShellTreeView.cpp` and `ShellBrowserImpl.cpp`.
- Keep build output quiet with `-v:m -clp:ErrorsOnly`. Run only the relevant tests with `--gtest_filter=`, then the full suite once with `--gtest_brief=1`.
- Load a skill only when the task matches it (see below).

## Build and test (Debug x64)
```powershell
$msbuild = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.Component.MSBuild -find 'MSBuild\Current\Bin\MSBuild.exe'
& $msbuild Explorer++\Explorer++.sln /t:Explorer++ /p:Configuration=Debug /p:Platform=x64 /m -v:m -clp:ErrorsOnly
# TestExplorer++ isn't in the Debug|x64 solution build; build the project directly.
& $msbuild Explorer++\TestExplorer++\TestExplorer++.vcxproj /p:Configuration=Debug /p:Platform=x64 "/p:SolutionDir=$((Resolve-Path Explorer++).Path)\" /m -v:m -clp:ErrorsOnly
.\Explorer++\TestExplorer++\x64\Debug\TestExplorer++.exe --gtest_brief=1 --gtest_filter=FooTest.*
```
The app builds to `Explorer++\Explorer++\x64\Debug\Explorer++.exe`. `Scripts\build-debug.ps1` (the VS Code build task) does the app build.

## Gotchas
- `Explorer++.rc`, `Explorer++VersionInfo.rc` and `TestResources.rc` are **UTF-16LE with a BOM, CRLF**. Edit them by decoding and re-encoding with `'utf-16'` (Python) or in the VS resource editor, never with a plain UTF-8 write. `Explorer++_NoTranslation.rc` is 8-bit.
- New `.cpp`/`.h` files need the GPL header from CONTRIBUTING.md, plus entries in **both** `.vcxproj` and `.vcxproj.filters`.
- Use the `.clang-format` style: Allman braces, tabs, 100 columns, no Hungarian notation in new code.
- Load UI text from the string table (`IDS_*`), not hard-coded English. Create dialogs through `ResourceLoader` so that dark mode applies.
- Record user-visible changes in `Documentation\User\History.txt`, with lines of 65 characters or fewer.

## Done means
The app builds, the relevant tests pass (plus new tests for new logic, in `TestExplorer++\<File>Test.cpp`), and `History.txt` is updated if users would notice. For UI changes, check both light and dark themes.

## Layout
| Path | What |
| --- | --- |
| `Explorer++\Explorer++\` | App sources (`ShellBrowser\`, `ShellTreeView\`, `Bookmarks\`, `Plugins\`, `DisplayWindow\`, `res\`) |
| `Explorer++\Helper\` | Static library of Win32/shell helpers |
| `Explorer++\TestExplorer++\` | GoogleTest/gMock tests (has its own `AGENTS.md`). `TestHelper\` is legacy, so don't use it |
| `Documentation\User\` | `History.txt` and the Sphinx user docs |
| `.github\skills\` | On-demand skills |

## Skills (`.github\skills\`)
- `explorerplusplus-architecture`: startup, `AppServices`, windows, tabs, navigation, events, threading
- `explorerplusplus-build-test`: build configurations, CI, test fixtures and troubleshooting
- `explorerplusplus-config-settings`: adding or changing persisted settings and Options pages
- `explorerplusplus-win32-ui`: dialogs, `.rc`, menus, accelerators, icons, dark mode, DPI

## MCP servers (root `.mcp.json`)
Microsoft Learn covers the Win32, Shell and COM APIs, plus C++, the standard library and MSVC compiler and linker diagnostics; check it before guessing at signatures or errors. GitHub covers upstream `derceg/explorerplusplus` history. Context7 covers library docs (Boost, concurrencpp, WIL, CLI11, GoogleTest).
