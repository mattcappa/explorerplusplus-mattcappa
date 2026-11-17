# Requirements

- Stable Visual Studio 2026 18.12 or later, or its standalone Build Tools,
  with MSVC 14.52 or later and the C++ x86/x64 build tools. A full Visual
  Studio IDE is not required.
- A Windows SDK with `NTDDI_WIN10_NI` (Windows 11 22H2) support.
- (Optional) To build with Clang (using the Debug-Clang/Release-Clang solution configurations), the following Visual Studio components should be installed:
    - C++ Clang tools for Windows
- (Optional) To build the installer, WiX 3.11 or later build tools are
  required. See the [WiX website](https://wixtoolset.org/docs/wix3/).

# Setup

[vcpkg](https://vcpkg.io/) is used to manage dependencies. Before you can build Explorer++, you'll first need to initialize vcpkg:

- Clone this repo with submodules enabled:
```
git clone --recurse-submodules https://github.com/derceg/explorerplusplus.git
cd explorerplusplus
```
- Run the vcpkg bootstrapper:
    - cmd/PowerShell: `.\Explorer++\ThirdParty\vcpkg\bootstrap-vcpkg.bat`
    - git bash/posix shell: `./Explorer++/ThirdParty/vcpkg/bootstrap-vcpkg.sh`


The relevant packages should then be automatically installed during the first build.

# Compiling

Run `.\Scripts\build-debug.ps1` from PowerShell. It selects a complete stable
VS 2026 18.12 or later installation and builds the Debug x64 app.

# Building and running from VS Code

The Agents window's Run menu provides **Build Explorer++ (Debug x64)** and **Build and Run Explorer++ (Debug x64, no debugger)**. Both use `Scripts\build-debug.ps1`, which discovers MSBuild and checks the vcpkg prerequisites. Build and Run passes `-Run` to the script and launches the application only after a successful build. This deliberately avoids `dependsOn`: Agents combines task dependencies with `&&`, which Windows PowerShell 5.1 does not support.

For breakpoint debugging, install the Microsoft C/C++ extension, open the workspace in the VS Code editor, select **Explorer++ (Debug x64)** in Run and Debug, and press F5. The launch configuration builds the application before starting the debugger. The Agents Run tasks launch without a debugger.

# Tests

The `TestExplorer++` project contains GoogleTest unit tests. Debug x64 tests
are not included in the solution build: build the project directly and pass
`SolutionDir`, then run the resulting `TestExplorer++.exe`. See `AGENTS.md`
for the exact commands.

Note that the `TestHelper` project is older and is in the process of being removed. It doesn't currently compile and shouldn't be used.
