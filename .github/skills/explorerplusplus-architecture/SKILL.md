---
name: explorerplusplus-architecture
description: Explorer++ architecture and cross-component patterns. Use when tracing or designing changes across application services, windows, tabs, ShellBrowser, navigation, events or threading.
---

# Explorer++ architecture

Explorer++ is a native Win32/Shell/COM file manager with no UI framework. The code is mid-refactor: newer interfaces, models, views and presenters coexist with legacy `Explorerplusplus` and `*Handler.cpp` code. Follow the nearest component's established pattern; prefer the newer, testable design.

## Application and services

- `WinMain.cpp` parses options through `CommandLine.cpp`, then runs `App`.
- `App` owns application-lifetime services and configures them before restoring windows and tabs. Settings load first; `SessionRestorer` restores the session.
- Pass the non-owning `AppServices` service locator to high-level classes instead of `App`; this keeps dependencies explicit and objects testable.
- For a new app-lifetime service, wire it through `App`, `AppServices` (including its service tuple), `App::SetUpAppServices()` and `BrowserTestBase::SetUpAppServices()`.

## Windows, tabs and navigation

```text
App -> BrowserList -> BrowserWindow
                    -> Explorerplusplus (main window)
                       -> BrowserPane -> Tab -> ShellBrowserImpl
                       -> ShellTreeView, DisplayWindow, toolbars, menus
```

- `BrowserWindow` is the app-facing interface; `Explorerplusplus` implements it. Prefer interfaces and factories (`BrowserWindowFactory`, `ShellBrowserFactory`) to concrete implementations.
- `TabList` tracks tabs across windows; `TabEvents` publishes tab lifecycle and selection changes.
- `ShellBrowser` is the per-tab folder view; `ShellBrowserImpl` handles enumeration, columns, sorting, filtering, view modes and directory changes.
- Navigation belongs in `ShellNavigationController` and `NavigationRequest`; `NavigationEvents` reports its lifecycle.
- `Explorerplusplus` is legacy and split across message/command handler files. Avoid adding more unrelated responsibilities there; extract focused logic where the nearby code supports it.

## Models, observers and UI

- Keep models independent of views. Views/presenters observe models (see bookmarks, toolbars and the tab bar).
- Models and event sources use `boost::signals2`/`SignalWrapper`. Store observer connections as `boost::signals2::scoped_connection` members so they disconnect with the observer.
- Experimental functionality is gated by `FeatureList`/`Feature.h`; check the existing feature pattern before adding a feature switch.

## Threading

- UI work runs on the single STA thread. Runtime provides UI, COM-STA, inline and timer executors.
- Shell work may use the COM-STA executor. Before touching windows or UI-owned models, resume on the UI executor (use the existing `concurrencpp::result` and `resume_on` patterns in `RuntimeHelper.h`).
- Async tests use `ExecutorTestBase` and `RuntimeTestHelper` (see the `explorerplusplus-build-test` skill).

## Subsystems and references

- Bookmarks, history, frequent locations, colour rules, drives and the applications toolbar have model/storage layers; prefer their existing model/view/storage pattern.
- Plugins use Lua/sol2 and are gated by `Feature::Plugins`.
- Shared Win32/Shell helpers live in `Explorer++\\Helper\\`. Third-party dependencies are in root `vcpkg.json`; use Context7 for their APIs.
- For design rationale, search upstream `derceg/explorerplusplus` commits/PRs. Check Microsoft Learn for Win32, Shell and COM API behavior.
