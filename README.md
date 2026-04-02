# MCTS-Allocator

`MCTS-Allocator` is a C++ project that applies **Monte Carlo Tree Search (MCTS)** to solve allocation/decision problems.  
The goal is to explore possible actions and select high-quality allocations based on simulation outcomes.

---

## Requirements

- **CMake** 3.16 or newer
- **C++17** compatible compiler

### Compilers by OS
- **Windows**: MSVC (Visual Studio 2022 recommended) or MinGW
- **Linux**: GCC or Clang
- **macOS (AppleOS)**: Apple Clang (Xcode Command Line Tools)

---

## Build with CMake

### 1) Configure

From project root:

```bash
cmake -S . -B build
```

### 2) Build

```bash
cmake --build build
```

### 3) Run

Executable name depends on your `CMakeLists.txt` target.  
Typical example:

- Linux/macOS: `./build/MCTS-Allocator`
- Windows: `.\build\Debug\MCTS-Allocator.exe` or `.\build\Release\MCTS-Allocator.exe`

---

## OS-Specific Quick Commands

### Windows (Visual Studio 2022)

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
.\build\Release\MCTS-Allocator.exe
```

### Linux

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/MCTS-Allocator
```

### macOS (AppleOS)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/MCTS-Allocator
```

If developer tools are missing on macOS:

```bash
xcode-select --install
```

---

## Optional: Run Tests

If tests are enabled in `CMakeLists.txt`:

```bash
ctest --test-dir build --output-on-failure
```

---

## Project Layout 

```text
MCTS-Allocator/
├── CMakeLists.txt
├── include/
├── src/
├── data/
└── README.md
```

---

## Notes

- Use `cmake --version` to verify CMake installation.
- If build fails, remove the build folder and reconfigure:
  - Linux/macOS: `rm -rf build`
  - Windows PowerShell: `Remove-Item -Recurse -Force build`