# MCTS Allocator

MCTS Allocator is a C++17 project that explores resource allocation with a Monte Carlo Tree Search (MCTS) approach. The program builds a search tree of possible allocations, evaluates candidate solutions with a configurable scoring function, and reports the best allocation it finds for a given problem size.

This repository is intentionally focused on the core idea of the algorithm and on how to run it in a portable way with CMake. It does not aim to document advanced tuning or performance workarounds.

## Quick Start

If you just want to build and run the project for the first time, the shortest path is:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

On Windows with Visual Studio, make sure the build tree is x64, for example:

```bash
cmake -S . -B build -A x64 -DCMAKE_BUILD_TYPE=Release
```

If you already configured `build/` as Win32, remove that build directory or use a fresh one before reconfiguring.

Then run the executable:

- Linux or macOS: `./build/mcts_main`
- Windows PowerShell: `.\build\Release\mcts_main.exe`

If your generator places the binary in a different folder, use that path instead. The program will read `config.toml` automatically, or generate a default one if the file is missing.

## Concept

The project models a simple allocation problem:

- there are a fixed number of agents,
- there are a fixed number of objects,
- each agent has a preference score for each object,
- the algorithm searches for an allocation that maximizes the overall score.

Instead of enumerating every possible allocation directly, the application uses Monte Carlo Tree Search:

1. It starts from an empty or partial allocation.
2. It expands the search tree by exploring candidate decisions.
3. It simulates completions of the current partial solution.
4. It propagates the obtained score back through the tree.
5. After a given number of iterations, it returns the best allocation found.

In practical terms, the project is useful when you want a reproducible way to test how MCTS can be applied to a combinatorial allocation problem.

## Repository Layout

- `src/` contains the application entry point and the implementation files.
- `include/` contains public headers for the MCTS core, configuration, and utility code.
- `config.toml` stores the default runtime configuration.
- `CMakeLists.txt` defines the build targets.
- `data/` is reserved for generated or example data.
- `results/` is created at runtime when result export is enabled.

## Requirements

You need the following tools installed:

- CMake 3.14 or newer
- A C++17-capable compiler
- A build tool supported by CMake for your platform

Typical compiler choices are:

- Windows: Visual Studio 2022, MSVC, or clang-cl
- Linux: GCC or Clang
- macOS: Clang from Xcode Command Line Tools

OpenMP is detected automatically when available. If your compiler supports it, the core library will link against it. If not, the project still configures, but without OpenMP support.

## Build With CMake

The project is designed to be built the same way on every operating system: configure a build directory, then build the chosen target.

### Step 1: Create a build directory

From the repository root, create a separate build directory. The exact name is up to you.

### Step 2: Configure the project

Run CMake configuration with a build type such as Release.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
```

Notes:

- On single-config generators such as Ninja, Unix Makefiles, or Makefiles on Linux/macOS, `CMAKE_BUILD_TYPE=Release` selects the Release configuration.
- On multi-config generators such as Visual Studio or Xcode, the build type is selected at build time instead. In that case, the configure step can stay generic.

### Step 3: Build the project

Use CMake to build the main executable.

```bash
# Basic run based purely on what's defined in config.toml
./mcts_main
```

### Command-Line Arguments (CLI)

You can easily override your configuration directly from the terminal without mutating your `config.toml`:

```bash
./mcts_main [OPTIONS]

Options:
  -h,--help                    Print this help message and exit
  -n,--num-agents INT          Override the number of agents
  -o,--num-objects INT         Override the number of objects
  -i,--iterations INT          Override the number of MCTS iterations to execute
  -e,--exploration FLOAT       Override the exploration constant (C in UCB1 formula)
  -t,--threads INT             Override the number of allowed threads (OpenMP)
  -s,--seed INT                Override the random seed used for generating preferences
  -v,--verbose                 Enable verbose outputs for step-by-step debugging
  -r,--ratio-random FLOAT      Determine the distribution ratio of random rollouts vs heuristic ones
```

### Config.toml

At launch, the engine attempts to load configurations from `config.toml` located in the execution directory. If the file does not exist, the engine will safely generate a default one:

```toml
[mcts]
exploration_constant = 1.414  # Corresponds generally to sqrt(2)
iterations = 100              # Budget or max search loop allowance
num_agents = 3
num_objects = 4
num_threads = -1              # -1 dictates 'Use all physically available core threads'
parallel_run = false          # Global parallelization toggling
seed = -1389484284            # Deterministic RNG seed. Change to diversify preferences.
verbose = false               # Keep terminal logs sparse (better performance)
```

## Structure OVERVIEW

- `src/main.cpp`: Executable entry point handling CLI configurations and TOML integration.
- `src/mcts/`: Contains the fundamental implementations (`MCTS.cpp`, `Node.cpp`, `Allocation.cpp`, `UCB.cpp`).
- `include/`: Holds project-wide headers alongside third-party headers (CLI11).
- `metrics/Utility.hpp`: Computes the actual "Reward" based upon agent preferences metrics ensuring fairly distributed systems.
