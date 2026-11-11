# Explorer++ (internal IT technician build)

This repository is an internal fork of [Explorer++](https://github.com/derceg/explorerplusplus), a Win32/C++ file manager. It's used only by IT technicians, so it doesn't need to serve a general audience.

## Product direction

- **English only.** Translation DLLs, the `Language` option, the `--language` switch and right-to-left (RTL) layout support have been removed. Don't reintroduce localisation plumbing, `LAYOUT_RTL` handling or per-language resources. OS-driven formatting of dates, numbers and sizes stays, because it follows the technician's Windows settings.
- **Windows 11 is the eventual target.** A later phase will remove compatibility code for older Windows versions. Until then, don't remove such code unless asked. New code may assume Windows 10 or later APIs only when the project already does so.
- Keep changes surgical and consistent with the existing code. The upstream author's conventions in [CONTRIBUTING.md](../CONTRIBUTING.md) still apply.

## Repository layout

| Path | Purpose |
| --- | --- |
| `Explorer++\Explorer++.sln` | Solution with the `Explorer++`, `Helper`, `TestExplorer++`, `TestHelper` (legacy) and `Installer` (WiX) projects |
| `Explorer++\Explorer++` | Application sources, including `ShellBrowser\`, `ShellTreeView\`, `Bookmarks\`, `Plugins\` (Lua), `DisplayWindow\` and `res\` |
| `Explorer++\Helper` | Static library of general Win32/shell helpers |
| `Explorer++\TestExplorer++` | GoogleTest/gMock unit tests, the main test project |
| `Explorer++\ThirdParty\vcpkg` | vcpkg submodule; dependencies are listed in the root `vcpkg.json` |
| `Documentation\User` | `History.txt` (changelog) and the Sphinx user docs |
| `Scripts\build-debug.ps1` | Optional local Debug x64 build helper used by the VS Code F5 task (may not be present in every checkout) |
| `.github\` | CI workflow, packaging and test scripts |

## Build and test

```powershell
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$msbuild = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -find 'MSBuild\Current\Bin\MSBuild.exe'

# App only (Debug x64). .\Scripts\build-debug.ps1 does the same, if present.
& $msbuild Explorer++\Explorer++.sln /t:Explorer++ /p:Configuration=Debug /p:Platform=x64 /m

# Unit tests. TestExplorer++ isn't in the Debug|x64 solution build, so build its project directly.
& $msbuild Explorer++\TestExplorer++\TestExplorer++.vcxproj /p:Configuration=Debug /p:Platform=x64 "/p:SolutionDir=$((Resolve-Path Explorer++).Path)\" /m
.\Explorer++\TestExplorer++\x64\Debug\TestExplorer++.exe --gtest_brief=1
```

## Conventions

- New `.cpp`/`.h` files start with the GPL license header from CONTRIBUTING.md. Add them to both the `.vcxproj` and the `.vcxproj.filters` file.
- Format with the root `.clang-format`: Allman braces, tabs, 100 columns. Don't use Hungarian notation in new code.
- Tests go in `TestExplorer++\<File>Test.cpp` and use `pch.h`.
- Record user-visible changes in `Documentation\User\History.txt`, keeping lines to 65 characters or fewer.
- `Explorer++.rc` and `TestResources.rc` are **UTF-16LE with a BOM** and CRLF line endings. Edit them with an encoding-aware tool, never with a naive UTF-8 write.

## Skills

The task-specific guidance in `.github\skills\` is loaded on demand:

- `explorerplusplus-architecture`: object model, startup, services, events and threading
- `explorerplusplus-build-test`: building, running and troubleshooting builds and tests
- `explorerplusplus-config-settings`: adding or changing persisted settings and options pages
- `explorerplusplus-win32-ui`: dialogs, menus, commands, resources, theming and DPI
- `explorerplusplus-testing`: writing tests with the existing fakes and fixtures

## MCP servers

`.vscode\mcp.json` configures three servers:

- **Microsoft Learn**: Win32, Shell and COM API documentation.
- **GitHub**: upstream `derceg/explorerplusplus` history, issues and PRs.
- **Context7**: library docs for Boost, concurrencpp, WIL, CLI11, GoogleTest and others.

Check the Win32 API with Microsoft Learn before guessing at signatures or behaviour.
