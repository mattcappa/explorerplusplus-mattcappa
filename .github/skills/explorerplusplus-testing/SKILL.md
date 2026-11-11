---
name: explorerplusplus-testing
description: How to write and run unit tests for Explorer++ in the TestExplorer++ GoogleTest/gMock project, including file naming, fixtures (BrowserTestBase, RegistryStorageTest, XmlStorageTest, ExecutorTestBase), fakes (BrowserWindowFake, ShellBrowserFake, ResourceLoaderFake), simulated file system, test resource files and storage round-trip tests. Use when adding or fixing tests or making code testable.
---

# Testing Explorer++

## Project basics

- Tests live in `Explorer++\TestExplorer++`, use **GoogleTest + gMock** (from vcpkg) and build to a single `TestExplorer++.exe`.
- The project references `Explorer++.vcxproj` and adds `Explorer++\Explorer++\` to the include path, so tests include app headers directly (`#include "AddressBar.h"`, `#include "ShellBrowser/ShellBrowser.h"`).
- Every test `.cpp` starts with the license header and then `#include "pch.h"`.
- **Naming:** tests for `Foo.cpp` go in `TestExplorer++\FooTest.cpp`, with fixture `FooTest`. Add new test files to both `TestExplorer++.vcxproj` and `TestExplorer++.vcxproj.filters`.
- Upstream policy (CONTRIBUTING.md): add tests whenever feasible. Code that's isolated from the filesystem and UI (parsing, models, storage, command logic) should always be tested. If you touch a file that has no test file yet, add one.
- See `explorerplusplus-build-test` for the build and run commands. `TestExplorer++` must be built through its `.vcxproj` in Debug.

## Choosing a fixture

| Need | Use |
| --- | --- |
| Pure logic or a model | plain `TEST` or `testing::Test` |
| Anything that needs browser windows, tabs, navigation or `AppServices` | `BrowserTestBase` (`BrowserTestBase.h`) |
| Registry storage | `RegistryStorageTest` (`RegistryStorageTestHelper.h`), which uses `HKCU\Software\Explorer++Test`, plus `ImportRegistryResource(L"foo.reg")` |
| XML storage | `XmlStorageTest` (`XmlStorageTestHelper.h`), with `CreateXmlDocument()` / `LoadXmlDocument(GetResourcePath(L"foo.xml"))` |
| Async code on concurrencpp executors | `ExecutorTestBase` (`ExecutorTestBase.h`) and `RuntimeTestHelper.h` |

`BrowserTestBase` builds a full fake application:

- Real models and events: `TabEvents`, `NavigationEvents`, `TabList`, `HistoryModel`, `BookmarkTree`, `Config`, `Win32ResourceLoader` and so on.
- Fakes for anything platform-specific: `PlatformContextFake`, `AsyncIconFetcherFake`, `DarkModeManagerFake`, `DriveWatcherFake`, `NoOpClipboardWatcher`, and `SimulatedFileSystem` with `SimulatedFileSystemWatcherFactory`.
- All of them wired into `m_appServices`.

Helpers on the fixture:

- `AddBrowser()` returns a `BrowserWindowFake *`, and `browser->AddTab(L"c:\\")` adds a tab to it.
- `NavigateTab(tab, path)` navigates a tab.

Example (`AddressBarTest.cpp`):

```cpp
class AddressBarTest : public BrowserTestBase
{
protected:
	AddressBarTest() :
		m_browser(AddBrowser()),
		m_addressBarView(std::make_unique<AddressBarView>(m_browser->GetView(), &m_config)),
		m_addressBar(std::make_unique<AddressBar>(m_addressBarView.get(), m_browser, &m_tabEvents,
			&m_shellBrowserEvents, &m_navigationEvents, &m_runtime, &m_iconFetcher))
	{
	}
	...
};
```

## Fakes and helpers available

- **Window, tab and shell:** `BrowserWindowFake`, `BrowserWindowMock`, `BrowserWindowFactoryFake`, `ShellBrowserFake`, `ShellBrowserFactoryFake`, `ShellEnumeratorFake`, `ShellIconLoaderFake`, `ShellNameParserFake`, `ShellContextFake`, `BrowserCommandTargetFake`.
- **Platform:** `PlatformContextFake`, `SystemClockFake`, `KeyboardStateFake`, `DriveEnumeratorFake`, `DriveWatcherFake`, `AppControllerFake`.
- **Resources and UI:** `ResourceLoaderFake`, `IconFetcherFake`, `MenuHelpTextHostFake`, `MenuViewTestHelper`, `ControlTestHelper`, `ListViewModelFake`, `TreeViewNodeFake`.
- **Shell items:** `PidlTestHelper.h` provides `CreateSimplePidlForTest(path)`, which makes simple PIDLs without touching the real filesystem.
- **Resources:** `ResourceTestHelper.h` provides `GetResourcePath(L"name")`. Files in `TestExplorer++\Resources\` are copied next to the exe by a post-build `xcopy`. `TestResources.rc` (UTF-16LE) embeds `IDB_BASIC_PNG`.
- **Storage round trips:** each storage area has a `*StorageTestHelper` (`ConfigStorageTestHelper::BuildReference()`, `BookmarkStorageTestHelper`, `TabStorageTestHelper`, …) that builds a non-default reference object. The XML and registry tests then save it, load it and compare with `EXPECT_EQ`. Add new fields to the reference builder so the round-trip tests cover them.
- **Mocks:** use gMock `MockFunction<void(...)>` with `EXPECT_CALL` to check signal and observer callbacks. Use `*Mock` classes such as `NavigationRequestDelegateMock` or `ApplicationExecutorMock` for interfaces.

## Making code testable

- Depend on interfaces (`BrowserWindow`, `ShellBrowser`, `ResourceLoader`, `DarkModeManager`, `PlatformContext`, `ShellEnumerator`, `DirectoryWatcherFactory`…) and pass them through constructors or `AppServices`, not `App` or `Explorerplusplus`.
- Keep Win32 calls in thin `*View` classes and put logic in models or controllers. This is the split used by the toolbars, the address bar, the tab bar and bookmarks.
- Inject time through `PlatformContext::GetSystemClock()` (faked by `SystemClockFake`).

## Gotchas

- Tests run on an STA thread with COM initialised. Shell APIs work, but prefer `SimulatedFileSystem` and fake PIDLs to touching real paths.
- Registry tests must not touch `HKCU\Software\Explorer++`, the technician's real settings.
- Don't put new tests in the legacy `TestHelper` project.
