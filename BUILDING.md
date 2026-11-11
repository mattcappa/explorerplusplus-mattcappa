# Requirements

- Visual Studio 2019/2022 (2022 recommended). Install the "Desktop development with C++" workload.
- Windows 10 SDK (most recent version recommended)
- (Optional) To build for ARM64, the following Visual Studio components should be installed:
    - MSVC v143 - VS 2022 C++ ARM64 build tools (Latest)
- (Optional) To build with Clang (using the Debug-Clang/Release-Clang solution configurations), the following Visual Studio components should be installed:
    - C++ Clang tools for Windows
- (Optional) To build the installer, WiX 3.11 and the relevant Visual Studio extension should be installed. Download links for both of those items can be found on the [WiX website](https://wixtoolset.org/docs/wix3/).

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

Open `Explorer++\Explorer++.sln`. From within Visual Studio, select `Debug` > `Start Without Debugging` to compile and run the program.

# Building and running from VS Code

The Agents window's Run menu provides **Build Explorer++ (Debug x64)** and **Build and Run Explorer++ (Debug x64, no debugger)**. Both use `Scripts\build-debug.ps1`, which discovers MSBuild and checks the vcpkg prerequisites. Build and Run passes `-Run` to the script and launches the application only after a successful build. This deliberately avoids `dependsOn`: Agents combines task dependencies with `&&`, which Windows PowerShell 5.1 does not support.

For breakpoint debugging, install the Microsoft C/C++ extension, open the workspace in the VS Code editor, select **Explorer++ (Debug x64)** in Run and Debug, and press F5. The launch configuration builds the application before starting the debugger. The Agents Run tasks launch without a debugger.

# Tests

The `TestExplorer++` project contains unit tests for the solution as a whole. The GoogleTest package is installed via vcpkg, so provided vcpkg has been initialized and you've been able to build the solution, you should just need to build the `TestExplorer++` project, then run the tests via the Visual Studio Test Explorer.

Note that the `TestHelper` project is older and is in the process of being removed. It doesn't currently compile and shouldn't be used.
