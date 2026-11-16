# AGENTS.md: TestExplorer++

These test rules add to the root [AGENTS.md](../../AGENTS.md). For more detail, load the `explorerplusplus-build-test` skill.

- Tests for `Foo.cpp` go in `FooTest.cpp` with fixture `FooTest`. Start the file with the GPL header, then `#include "pch.h"`, and add it to both `TestExplorer++.vcxproj` and its `.filters` file.
- Choose a fixture:
  - `BrowserTestBase`: windows, tabs or `AppServices`.
  - `XmlStorageTest` / `RegistryStorageTest`: storage.
  - `ExecutorTestBase`: async code.
  - Plain `TEST`: pure logic.
- Use the existing `*Fake`/`*Mock` classes before writing new ones. Use `SimulatedFileSystem` and `PidlTestHelper` instead of real paths.
- Registry tests use only `HKCU\Software\Explorer++Test`, never `HKCU\Software\Explorer++`.
- When you add a setting, give it a non-default value in `ConfigStorageTestHelper.cpp` `BuildReference()`, so the round-trip tests cover it.
- `TestResources.rc` is UTF-16LE with a BOM. Files in `Resources\` are copied next to the exe after the build; load them with `GetResourcePath()`.
- Run only what you touched: `x64\Debug\TestExplorer++.exe --gtest_filter=FooTest.*`.
