# AI Verification Studio

AI Verification Studio is a C++ Qt desktop application that integrates LLM APIs for automatically generating, simulating, and evaluating SystemVerilog code from hardware specifications.

## Code Structure

- **`src/main.cpp`**: Entry point of the application.
- **`src/MainWindow.cpp` / `MainWindow.h`**: The primary Qt graphical UI, orchestrating user configurations, text/PDF specification parsing, test plan generation, code generation, and the automated AI debugging loops. 
- **`src/LlmClient.cpp` / `LlmClient.h`**: Manages asynchronous communication with remote LLM APIs (Google Gemini, OpenRouter, Codestral). Formats JSON payloads, processes streamed generation, handles timeout timers, and returns completed textual test plans, code snippets, or markdown reports.
- **`src/SimulationRunner.cpp` / `SimulationRunner.h`**: Exclusively handles the SystemVerilog compilation (`iverilog`) and simulation execution (`vvp`), returning captured standard output and errors via signals to dictate the AI refactoring loop.
- **`compile.py`**: A cross-platform automated build wrapper that dynamically passes system environment variables to `qt-cmake.bat` and standard `cmake` builders to ensure successful builds.

## Dependencies

To build and run this project, you will need the following dependencies installed on your system:
- **C++17 Compatible Compiler**: MSVC (Windows), MinGW-w64, GCC, or Clang.
- **CMake**: Version 3.16 or higher.
- **Python 3.x**: Specifically to run the `compile.py` wrapper.
- **Qt 6**: Specifically the Core, Gui, Widgets, Network, and Concurrent modules.
- **Icarus Verilog (`iverilog` / `vvp`)**: Required functionally in your system `PATH` to automatically run and test generated SystemVerilog logic paths.
- **API Keys**: Access keys to Gemini, Codestral, or OpenRouter for the text generation loop.

## Instructions to Run

1. Open a system terminal/PowerShell in the repository root directory.
2. Run the Python build script:
   ```bash
   python compile.py
   ```
3. The script will automatically link Qt environment dependencies using CMake, and outputs the executable via Ninja. The `.exe` or executable binary will be generated directly into the newly created `build/` directory.
4. Execute the desktop app directly:
   ```bash
   .\build\ai_verif_studio.exe
   ```
   *(Ensure Qt DLLs are included using `windeployqt` which the CMakeLists file executes automatically).*

## Code Provenance




**Adapted from Prior Code:**
- Structural scaffolding such as `src/main.cpp` and initial portions of `MainWindow.cpp` (setup routines and base widget initializations, approx Lines 1 to 200) were instantiated directly from a bare-minimum Qt layout framework block prior to AI expansion.
- Initial baseline of `LlmClient.cpp`'s QNetworkAccessManager instance code was adapted from the prior baseline, before being extended to handle multi-model processing endpoints.

**Datasets & External Models Verification Policy:**
- No static datasets or pre-trained models are downloaded or stored algorithmically. 
- Input context strictly relies on user-provided `.txt` specifications or user-uploaded design parameter `.pdf` documents within the GUI.
- AI Models (Gemini, Codestral, OpenRouter) are strictly accessed remotely over the network via standard API hooks. There are no locally downloaded LLM files or proprietary weights utilized explicitly due to their size footprint, eliminating the need for automated resource downloads by the software tool.
