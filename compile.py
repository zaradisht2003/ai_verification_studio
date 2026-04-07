import subprocess
import os

env = os.environ.copy()
env["PATH"] = "C:\\Qt\\6.10.2\\mingw_64\\bin;C:\\Qt\\Tools\\mingw1310_64\\bin;C:\\Qt\\Tools\\Ninja;C:\\Qt\\Tools\\CMake_64\\bin;" + env.get("PATH", "")

result = subprocess.run(["cmake", "--build", "build"], env=env, capture_output=True, text=False)

with open('compile_out.txt', 'wb') as f:
    f.write(result.stdout)
with open('compile_err.txt', 'wb') as f:
    f.write(result.stderr)

print("Done.")
