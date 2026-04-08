import subprocess
import os

env = os.environ.copy()
# Ensure all Qt, MinGW, CMake, and Ninja tools are in PATH
env["PATH"] = "C:\\Qt\\6.10.2\\mingw_64\\bin;C:\\Qt\\Tools\\mingw1310_64\\bin;C:\\Qt\\Tools\\Ninja;C:\\Qt\\Tools\\CMake_64\\bin;" + env.get("PATH", "")

# 1. Configure the project into the 'build' directory
print("Configuring the project...")
config_cmd = '"C:\\Qt\\6.10.2\\mingw_64\\bin\\qt-cmake.bat" -G Ninja -S . -B build'
config_result = subprocess.run(config_cmd, env=env, capture_output=True, text=False, shell=True)

# Write configuration output
with open('compile_out.txt', 'wb') as f:
    f.write(b"--- CONFIGURATION OUTPUT ---\n")
    f.write(config_result.stdout)
    f.write(b"\n")
with open('compile_err.txt', 'wb') as f:
    f.write(b"--- CONFIGURATION ERROR ---\n")
    f.write(config_result.stderr)
    f.write(b"\n")

if config_result.returncode != 0:
    print(f"Configuration failed (Exit Code {config_result.returncode}). Check compile_err.txt")
    exit(1)

# 2. Build the project
print("Building the project in 'build' directory...")
build_cmd = 'cmake --build build'
# Added shell=True and passed as string so cmd.exe can correctly resolve cmake.exe using the newly constructed env path
build_result = subprocess.run(build_cmd, env=env, capture_output=True, text=False, shell=True)

# Append build output
with open('compile_out.txt', 'ab') as f:
    f.write(b"--- BUILD OUTPUT ---\n")
    f.write(build_result.stdout)
with open('compile_err.txt', 'ab') as f:
    f.write(b"--- BUILD ERROR ---\n")
    f.write(build_result.stderr)

if build_result.returncode != 0:
    print("Build failed. Check compile_err.txt")
    exit(1)

print("Done. Executable should be in the 'build' directory.")
