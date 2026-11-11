---
name: explorerplusplus-architecture
description: Explorer++ architecture reference covering startup (WinMain, App, AppServices), browser windows, tabs, ShellBrowser, navigation, events (boost::signals2), threading (concurrencpp, UI thread executor), plugins and bookmarks. Use when locating code, designing a change that crosses components, or explaining how Explorer++ works internally.
---

# Explorer++ architecture

Explorer++ is a native C++ file manager that calls the Windows API, Shell and COM directly, with no UI framework. The code is midway through a long-running upstream refactor: newer, testable abstractions such as `AppServices`, `BrowserWindow`, `ShellBrowser`, models with signals and `*View` classes sit alongside older code such as `Explorerplusplus`, the `*Handler.cpp` files and raw `WM_COMMAND` handling. When you change code, follow the newer pattern used by the nearest component.

All paths below are relative to `Explorer++\Explorer++\` unless stated otherwise.

## Startup

1. `WinMain.cpp`: `WinMain` parses the command line (`CommandLine.cpp`, which uses CLI11), then calls `App app(&commandLineSettings); return app.Run();`.
2. `App.h`/`App.cpp`: `App` owns every application-lifetime service as a direct member: `Runtime`, `Config`, `AcceleratorManager`, `DarkModeManagerImpl`, `ThemeManager`, `BrowserList`, `TabList`, `TabEvents`, `ShellBrowserEvents`, `NavigationEvents`, `HistoryModel`, `BookmarkTree`, `ColorRuleModel`, `PluginManager` and others.
   - `App::Run` calls `SetUpSession()`, starts a 30-second save timer (`SaveSettings`), then runs the `GetMessage` loop. The loop gives modeless dialogs (`IsModelessDialogMessage`) and accelerators (`MaybeTranslateAccelerator`) first chance at each message.
   - `App::SetUpSession` runs these steps in order:
     1. `LoadSettings()` reads the XML config file if one is present, otherwise the registry (`XmlAppStorageFactory` / `RegistryAppStorageFactory`). Settings load before anything else.
     2. `ProcessManager::InitializeCurrentProcess` enforces single-instance behaviour.
     3. `SetUpAppServices()`.
     4. Jump list setup.
     5. `InitializePlugins()`.
     6. `SessionRestorer::Restore(windows)`, which recreates the saved browser windows and tabs.
   - `App::SetUpResources()` creates the `Win32ResourceLoader` and `BrowserWindowFactoryImpl` using the EXE module, `GetModuleHandle(nullptr)`. There are no satellite resource DLLs, because this fork is English only.
3. `AppServices.h` is a non-owning service locator. It has getters and setters for each service, plus `CheckFullyInitialized()`. The upstream author's comment in that file explains the design: pass `AppServices` to high-level classes rather than `App`, so they stay testable and constructors stay small. Tests populate it themselves (see `TestExplorer++\BrowserTestBase.h`). When you add a new application-lifetime service:
   1. Add a member to `App`.
   2. Add a getter and setter to `AppServices`, and add the type to its `m_services` tuple.
   3. Set the service in `App::SetUpAppServices()`.
   4. Set it in `BrowserTestBase::SetUpAppServices()`.

## Window and tab object model

```
App
└── BrowserList ──► BrowserWindow (interface, BrowserWindow.h)
                    └── Explorerplusplus (Explorer++.h; implements BrowserWindow + CoreInterface)
                        ├── BrowserPane (BrowserPane.h) ──► TabBar ──► Tab (Tab.h)
                        │                                              └── ShellBrowserImpl (ShellBrowser\)
                        │                                                   ├── ShellNavigationController
                        │                                                   └── NavigationManager / NavigationRequest
                        ├── ShellTreeView (ShellTreeView\) ── folders pane (FoldersPane)
                        ├── DisplayWindow (DisplayWindow\) ── preview/info pane
                        ├── MainRebar: MainToolbar, AddressBar, BookmarksToolbar, DrivesToolbar, ApplicationToolbar
                        ├── MainMenu (MenuBase) + BrowserCommandController
                        └── StatusBar
```

- `Explorerplusplus` is the concrete main window. It's split across `Explorer++.cpp`, `MsgHandler.cpp`, `TabHandler.cpp`, `ListViewHandler.cpp` and the other handler files. It's the oldest code; prefer to extract logic into small classes rather than grow it.
- `BrowserWindow` is the interface that the rest of the app and the tests use. `TestExplorer++\BrowserWindowFake.h` and `BrowserWindowMock.h` implement it. `BrowserWindowFactory`/`BrowserWindowFactoryImpl` create windows, and `BrowserList` tracks them.
- `TabList` tracks every tab across all windows. `TabEvents` broadcasts tab create, select, move, update and remove events.
- `ShellBrowser` (interface) and `ShellBrowserImpl` hold the list view of one folder in one tab. `ShellBrowser\` contains enumeration, columns (`ColumnDataRetrieval`, `ColumnManager`), sorting, grouping, filtering, view modes, thumbnails, drag/drop (`DropTarget.cpp`) and directory change handling (`DirectoryModificationHandler.cpp`). `ShellBrowserFactory` creates instances, so tests can substitute `ShellBrowserFake`.
- Navigation goes through `ShellNavigationController` (history, back/forward and `NavigateParams`) and `NavigationRequest`. `NavigationEvents` announces navigation start, commit, completion and failure.
- `FeatureList`/`Feature.h` gate experimental features (`DualPane`, `Plugins`, `AutomaticQuickAccessUpdates`, `MultipleWindowsPerSession`, `BackgroundThreadEnumeration`), which are enabled with `--enable-features`.

## Events and observers

- Model and event classes expose `boost::signals2` signals, often through `..\Helper\SignalWrapper.h` (which provides `AddObserver`) or `SignalHelper.h`. Examples are `TabEvents`, `ShellBrowserEvents`, `NavigationEvents`, `BrowserList::willRemoveBrowserSignal`, `HistoryModel` and `BookmarkTree`.
- Keep each returned connection in a `std::vector<boost::signals2::scoped_connection>` member, so the subscription ends when the observer is destroyed.
- Views observe models. Models don't know about views. Follow the `*Model` → `*View` / `*Presenter` split used by bookmarks, the toolbars and the tab bar.

## Threading

- The UI runs on a single STA thread. `Runtime` (`Runtime.h`) exposes concurrencpp executors:
  - `GetUiThreadExecutor()`: `UIThreadExecutor`, which posts work to the UI thread.
  - `GetComStaExecutor()`: a pool of COM STA worker threads (`ComStaThreadPoolExecutor`) for shell calls.
  - `GetInlineExecutor()` and `GetTimerQueue()`.
- Background work must return to the UI thread before it touches windows or models. Use `concurrencpp::result` coroutines with the `resume_on` helpers in `RuntimeHelper.h`. Existing usages are in `Explorer++.cpp`, `MsgHandler.cpp`, `FileSystemWatcher.cpp` and `ShellBrowser\DirectoryModificationHandler.cpp`. `AsyncIconFetcherImpl` and `ShellEnumeratorImpl` are other examples of off-UI-thread work.
- Tests drive executors manually with `TestExplorer++\ExecutorTestBase.h` and `RuntimeTestHelper.h`.

## Other subsystems

- **Bookmarks** (`Bookmarks\`): `BookmarkTree` model, `BookmarkItem`, XML and registry storage, and `Bookmarks\UI\` (presenters, menus, toolbar and the Manage Bookmarks dialog).
- **Plugins** (`Plugins\`): Lua through sol2. `PluginManager` loads each `plugins\<name>\plugin.json` manifest from the EXE folder. `ApiBinding.cpp` exposes the API. `PluginCommandManager` and `PluginMenuItemsRegistry` connect plugins to accelerators and menus. Plugins are behind `Feature::Plugins`.
- **Applications toolbar** (`Application*.cpp`): technician-defined launchers with their own model and storage.
- **Colour rules, history, frequent locations, drives**: each is a `*Model` with signals, plus XML and registry storage.
- **Helper library** (`Explorer++\Helper\`): a static library for shell and PIDL utilities (`Pidl.h`, `ShellHelper`), file operations, registry/XML settings helpers, `WindowSubclass`, `SignalWrapper` and `CachedIcons`.

## Third-party libraries

These are vcpkg manifest dependencies in the root `vcpkg.json`:

- **Boost**: signals2, multi-index, bimap and others.
- **concurrencpp**.
- **WIL**: `wil::unique_*` RAII and `THROW_IF_FAILED`.
- **glog**: `CHECK`/`DCHECK`/`LOG`.
- **CLI11**, **fmt**, **magic_enum**, **cereal**, **nlohmann-json**, **pegtl**.
- **Lua** and **sol2**.
- **cppwinrt**, **detours**.
- **GoogleTest** (tests only).

Use the Context7 MCP server for their docs.

## Upstream developer resources

- Upstream repo and history: https://github.com/derceg/explorerplusplus. Use the GitHub MCP server to read PRs and commits when you need the rationale for a design.
- Upstream wiki, "Useful references" for Win32 programming: catch22.net (custom Win32 controls), the 2xExplorer Shell FAQ (shell namespace and PIDLs), and Raymond Chen's *The Old New Thing*. See https://github.com/derceg/explorerplusplus/wiki/Useful-references.
- User documentation: https://explorerplusplus.readthedocs.io (source in `Documentation\User\Sphinx`).
- Win32, Shell and COM API reference: Microsoft Learn MCP server.
