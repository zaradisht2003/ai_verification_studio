# AI Verification Studio

AI Verification Studio is a C++ Qt desktop application that integrates LLM APIs for generating and analyzing SystemVerilog code.

## Prerequisites

To build and run this project, you will need the following dependencies installed on your system:

- **C++17 Compatible Compiler**: MSVC (Windows), MinGW-w64, GCC, or Clang.
- **CMake**: Version 3.16 or higher.
- **Qt 6**: Specifically the Core, Gui, Widgets, Network, and Concurrent modules.

## Building the Project

The project uses CMake for building. Based on the environment, MinGW and Ninja were used for a successful build. Follow these steps to compile the application from the terminal (e.g., PowerShell):

1. **Set up your environment variables**:
   Ensure that the Qt MinGW binary path, CMake, and Ninja are in your system's `PATH`. For example:
   ```powershell
   $env:PATH="C:\Qt\6.10.2\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;C:\Qt\Tools\Ninja;C:\Qt\Tools\CMake_64\bin;$env:PATH"
   ```

2. **Configure the project with CMake**:
   Use the Ninja generator and point `CMAKE_PREFIX_PATH` to your Qt installation:
   ```powershell
   cmake -G Ninja -S . -B build "-DCMAKE_PREFIX_PATH=C:\Qt\6.10.2\mingw_64"
   ```

3. **Build the executable**:
   ```powershell
   cmake --build build
   ```
   *Note: On Windows, building the executable will automatically trigger `windeployqt` as a post-build step to copy the required Qt DLLs directly next to the compiled executable in the `build` directory.*

## Running the Application

Once built successfully, the compiled executable will be located inside the `build` directory (or inside a configuration-specific subdirectory like `build/Debug`).

You can run the application directly from the command line:
```bash
# Depending on your compiler/generator, the executable might be directly in build/
.\ai_verification_studio.exe

# Or inside a Debug/Release folder for multi-config generators (like MSVC)
.\Debug\ai_verification_studio.exe
```

Alternatively, you can run the executable from your IDE (such as VS Code or Visual Studio) by using the built-in Run or Debug buttons.

## Troubleshooting

- **"Bad CMake executable"**: Ensure your CMake installation is in your system's `PATH` variable, or properly set `cmake.cmakePath` if you are using VS Code settings.
- **Qt Not Found**: Ensure you have installed the correct Qt 6 modules (Core, Gui, Widgets, Network, Concurrent). Use the Qt Maintenance Tool / Online Installer to add these if they are missing.
