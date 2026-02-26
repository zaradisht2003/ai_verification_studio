# AI Verification Studio

AI Verification Studio is a C++ Qt desktop application that integrates LLM APIs for generating and analyzing SystemVerilog code.

## Prerequisites

To build and run this project, you will need the following dependencies installed on your system:

- **C++17 Compatible Compiler**: MSVC (Windows), MinGW-w64, GCC, or Clang.
- **CMake**: Version 3.16 or higher.
- **Qt 6**: Specifically the Core, Gui, Widgets, Network, and Concurrent modules.

## Building the Project

The project uses CMake for building. Follow these steps to compile the application from the terminal:

1. **Open a terminal** and navigate to the project directory:
   ```bash
   cd path/to/ai_verification_studio
   ```

2. **Create a build directory**:
   ```bash
   mkdir build
   cd build
   ```

3. **Configure the project with CMake**:
   ```bash
   cmake ..
   ```
   *Note: You may need to provide the path to your Qt installation if CMake cannot find it automatically. You can do this by adding `-DCMAKE_PREFIX_PATH="C:\path\to\Qt\6.x.x\msvc2019_64"` (replace with your actual Qt path) to the cmake command.*

4. **Build the executable**:
   ```bash
   cmake --build .
   ```
   *Note: On Windows, building the executable will automatically trigger `windeployqt` as a post-build step to copy the required Qt DLLs directly next to the compiled executable.*

## Running the Application

Once built successfully, the compiled executable will be located inside the `build` directory (or inside a configuration-specific subdirectory like `build/Debug`).

You can run the application directly from the command line:
```bash
# Depending on your compiler/generator, the executable might be directly in build/
.\ai_studio_v2.exe

# Or inside a Debug/Release folder for multi-config generators (like MSVC)
.\Debug\ai_studio_v2.exe
```

Alternatively, you can run the executable from your IDE (such as VS Code or Visual Studio) by using the built-in Run or Debug buttons.

## Troubleshooting

- **"Bad CMake executable"**: Ensure your CMake installation is in your system's `PATH` variable, or properly set `cmake.cmakePath` if you are using VS Code settings.
- **Qt Not Found**: Ensure you have installed the correct Qt 6 modules (Core, Gui, Widgets, Network, Concurrent). Use the Qt Maintenance Tool / Online Installer to add these if they are missing.
