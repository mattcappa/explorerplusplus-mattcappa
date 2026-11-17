---
name: explorerplusplus-build-test
description: Explorer++ build setup, configurations, CI and GoogleTest/gMock test patterns. Use when building, changing build configuration, adding tests or troubleshooting failures.
---

# Building and testing Explorer++

Use the exact local build commands in the root `AGENTS.md`; this skill covers setup, project-specific gotchas and test patterns.

## Build setup

- Use stable Visual Studio 2026 18.12+ / Build Tools with MSVC 14.52+, **Desktop development with C++** and a Windows SDK.
- Initialise and bootstrap the `Explorer++\\ThirdParty\\vcpkg` submodule if needed. Root MSBuild props import vcpkg; the first build installs manifest dependencies to `vcpkg_installed\\`.
- Find MSBuild with `vswhere`; it is not necessarily on `PATH`. If using `vcvars`, run setup and build in the same `cmd /c` process.
- Configurations: `Debug`, `Debug-Asan`, `Debug-Clang`, `Release`, `Release-Clang`; platforms: `Win32`, `x64`.
- `Scripts\\build-debug.ps1` and the VS Code **Explorer++ (Debug x64)** launch configuration wrap the app build.
- The test project has no `Build.0` for solution `Debug|x64`; build its `.vcxproj` directly and pass `SolutionDir` (command in `AGENTS.md`).

## Tests

- Tests are GoogleTest/gMock in `Explorer++\\TestExplorer++`; `TestHelper` is legacy and should not receive new tests.
- Choose a fixture:

  | Need | Fixture |
  | --- | --- |
  | Pure logic/model | Plain `TEST` or `testing::Test` |
  | Windows, tabs, navigation or `AppServices` | `BrowserTestBase` |
  | XML or registry persistence | `XmlStorageTest` / `RegistryStorageTest` |
  | Async work | `ExecutorTestBase` and `RuntimeTestHelper` |

- `BrowserTestBase` wires real models with fakes for platform-dependent services. Use existing `*Fake`/`*Mock` types, `SimulatedFileSystem` and fake PIDLs instead of real paths where practical.
- For storage fields, add non-default values to the relevant `*StorageTestHelper::BuildReference()`; round-trip tests compare saved and restored objects. Add migration tests for renamed or migrated fields.
- Keep logic independently testable: depend on interfaces, isolate Win32 calls in views, and inject time through `PlatformContext::GetSystemClock()`.
- Tests run on an STA thread with COM initialized. Registry tests must use `HKCU\\Software\\Explorer++Test`, never technician settings.
- Test file naming, project entries and resource details are in `Explorer++\\TestExplorer++\\AGENTS.md`. Run only the relevant tests first; use the root `AGENTS.md` command.

## CI

- `.github\\workflows\\build.yml` builds `Debug-Asan|x64`, `Release-Clang|x64`, and `Release` for `Win32` and `x64`.
- Jobs bootstrap vcpkg, use its binary cache, build, run `.github\\run_tests.ps1`, and package artifacts.
- `.github\\workflows\\coverage.yml` runs the coverage configuration; `run_tests_with_coverage.ps1` collects Clang coverage.
- `TestExplorer++` output is under `Explorer++\\TestExplorer++\\<Platform>\\<Configuration>\\`.

## Troubleshooting

| Symptom | Likely fix |
| --- | --- |
| `vcpkg.props` missing | Initialise the submodule and bootstrap vcpkg |
| `MSB4057` for the test target | Build the test `.vcxproj` directly; see above |
| Garbled resource-compiler text | Restore the `.rc` encoding; see `explorerplusplus-win32-ui` |
| Unresolved external after adding a file | Add it to the relevant `.vcxproj` |
| `cl`/`link` missing | Run vcvars and MSBuild in the same process |

For build validation and expected behavior, follow "Done means" in root `AGENTS.md`.
