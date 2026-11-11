---
name: explorerplusplus-build-test
description: How to build, run, debug and test Explorer++ locally with MSBuild, vcpkg and the TestExplorer++ GoogleTest executable, plus how the CI matrix works and how to troubleshoot common build failures. Use when compiling, running unit tests, validating a change, or fixing build errors.
---

# Building and testing Explorer++

## Prerequisites

- Visual Studio 2022, or Build Tools 2022, with the **Desktop development with C++** workload and a Windows SDK.
- The vcpkg submodule at `Explorer++\ThirdParty\vcpkg`, initialised and bootstrapped:
  ```powershell
  git submodule update --init --recursive
  .\Explorer++\ThirdParty\vcpkg\bootstrap-vcpkg.bat
  ```
  The root `Directory.Build.props` and `Directory.Build.targets` import vcpkg's MSBuild integration. The first build installs the dependencies listed in `vcpkg.json` into `vcpkg_installed\`.

## Find MSBuild

Don't assume MSBuild is on `PATH`. Find it with vswhere:

```powershell
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$msbuild = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -find 'MSBuild\Current\Bin\MSBuild.exe'
```

## Build the app

```powershell
& $msbuild Explorer++\Explorer++.sln /t:Explorer++ /p:Configuration=Debug /p:Platform=x64 /m
```

- The output is `Explorer++\Explorer++\x64\Debug\Explorer++.exe`.
- Some checkouts have a local helper, `Scripts\build-debug.ps1`, that runs the same build, plus a VS Code **Explorer++ (Debug x64)** launch config (`.vscode\launch.json`) whose pre-launch task calls it. Use them if present; don't rely on them.
- Solution configurations are `Debug`, `Debug-Asan`, `Debug-Clang`, `Release` and `Release-Clang`. Platforms are `Win32`, `x64` and `ARM64`.
- The code is C++20 (`stdcpp20`) at warning level 4. Third-party headers have their warnings turned off.

## Build and run the unit tests

`TestExplorer++` is **not** built in the solution's `Debug|x64` configuration, which has `ActiveCfg` but no `Build.0`. The `/t:TestExplorer++` solution target therefore fails with MSB4057. Build the project file directly instead, and pass `SolutionDir`:

```powershell
& $msbuild Explorer++\TestExplorer++\TestExplorer++.vcxproj /p:Configuration=Debug /p:Platform=x64 "/p:SolutionDir=$((Resolve-Path Explorer++).Path)\" /m
.\Explorer++\TestExplorer++\x64\Debug\TestExplorer++.exe --gtest_brief=1
.\Explorer++\TestExplorer++\x64\Debug\TestExplorer++.exe --gtest_filter=ConfigXmlStorageTest.*
```

- CI runs `.github\run_tests.ps1 -Configuration <cfg> -Platform <plat>`, which runs `TestExplorer++.exe` from `Explorer++\TestExplorer++\<Platform>\<Configuration>\`. `run_tests_with_coverage.ps1` collects Clang coverage.
- The registry storage tests write to `HKCU\Software\Explorer++Test` and clean up afterwards.
- `TestHelper` is a legacy project. Upstream documents it as "being removed" and not compiling. CI doesn't build or run it, so don't add tests there.

## CI (`.github\workflows\build.yml`)

- The matrix covers `Debug-Asan|x64`, `Release-Clang|x64`, `Release|Win32`, `Release|x64` and `Release|ARM64`.
- Each job:
  1. Bootstraps vcpkg, using a cached binary cache.
  2. Builds the solution with MSBuild.
  3. Runs `run_tests.ps1`.
  4. Packages the artifacts with `package.ps1`, producing `explorerpp_<platform>.zip` and `.pdb` zips. There's no translations artifact any more.
- `deploy.ps1` uploads the artifacts. It's upstream infrastructure and can usually be ignored for internal builds.

## Validating a change

1. Build the app (Debug x64).
2. Build TestExplorer++ and run the tests that relate to the change, then the full suite. At the time of writing the baseline is 897 passing tests.
3. For UI changes, launch the exe and check both light and dark themes (Tools > Options > Appearance > Theme).
4. Make sure every new file is in both the `.vcxproj` and the `.vcxproj.filters` file.

## Troubleshooting

| Symptom | Fix |
| --- | --- |
| `vcpkg.props` not found | Initialise the submodule and bootstrap vcpkg |
| `MSB4057: target "TestExplorer++" does not exist` | Build `TestExplorer++.vcxproj` directly (see above) |
| RC compile errors such as garbled text or "unexpected end of file" | `Explorer++.rc` was saved in the wrong encoding. It must be UTF-16LE with a BOM. See `explorerplusplus-win32-ui` |
| Unresolved external after adding a file | The file is missing from the `.vcxproj` |
| Link errors about `vcpkg_installed` after changing `vcpkg.json` | Delete `vcpkg_installed\` and rebuild |
| `cl`/`link` not found in a separate shell | Run vcvars and the build in the **same** `cmd /c` invocation |
