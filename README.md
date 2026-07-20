# MCTS Allocator

MCTS Allocator is a C++17 project that explores resource allocation with a Monte Carlo Tree Search (MCTS) approach. The program builds a search tree of possible allocations, evaluates candidate solutions with a configurable scoring function, and reports the best allocation it finds for a given problem size.

This repository is intentionally focused on the core idea of the algorithm and on how to run it in a portable way with CMake.

## Quick Start

If you just want to build and run the project for the first time, the shortest path is:

```bash
# 1. Configure the project
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# 2. Build the project
cmake --build build --config Release

# 3. Run the executable
# On Linux or macOS:
./build/mcts_main

# On Windows (PowerShell):
.\build\Release\mcts_main.exe
```

*Note: The program will read `config.toml` automatically, or generate a default one if the file is missing.*

## Requirements

- **CMake** 3.14 or newer
- **C++17-capable compiler** (e.g., GCC, Clang, MSVC)
- **OpenMP** (Optional but recommended): Detected automatically for multi-threading support.
- **Gurobi** (Optional): Only if you want to build the `testLp` executable. Set the `GUROBI_HOME` environment variable to your Gurobi installation root (e.g., `C:\gurobi1301\win64`).

## Usage

### 1. The Main Executable (`mcts_main`)

You can run the application directly. It will use the settings defined in `config.toml`.

```bash
# Example for Linux/macOS
./build/mcts_main
```

You can easily override configuration parameters using command-line arguments:

```bash
./build/mcts_main [OPTIONS]

Options:
  -h,--help                             Print this help message and exit
  -n,--num-agents INT                   Override the number of agents
  -o,--num-objects INT                  Override the number of objects
  -i,--iterations INT                   Override the number of MCTS iterations
  -e,--exploration FLOAT                Override the exploration constant (C)
  -s,--seed INT                         Override the random seed for preference generation
  -r,--ratio-random FLOAT               Override the ratio of random simulations
  -t,--time-budget-seconds FLOAT        Override the time budget in seconds for MCTS
  -p,--selected-politic INT             Override the selected politic for determining the ratio of random simulations
  -A,--agent-have-minimum-one-object    Ensure each agent has at least one object in the allocation
  -B,--use-time-budget                  Use a time budget instead of a number of iterations for MCTS
  -G,--show-metrics                     Show metrics (EF, EFX, Prop, ...) for the best allocation after MCTS run
  -J,--terminal-json-output             Output results in JSON format to the terminal
  -M,--monitoring-cuts                  Enable monitoring of cuts to get how many cuts are made inside the tree
  -N,--uniformize-negative-values       Uniformize negative values in preferences
  -S,--save-results                     Save results to a JSON file in the results directory
  -T,--add-metrics-to-utility           Add metrics to the utility calculation (EF, EFX, Prop, etc.)
  -U,--use-solver                       Use the Gurobi solver to find the optimal allocation instead of MCTS
  -V,--verbose                          Enable verbose output for debugging
  -P,--show-progress                    Show progress information during the MCTS run
```

### 2. Configuration File (`config.toml`)

At launch, the engine attempts to load configurations from a `config.toml` file located in the current execution directory. If the file does not exist, a default one is automatically generated:

```toml
[mcts]
exploration_constant = 1.414  # Corresponds generally to sqrt(2)
iterations = 100              # Budget or max search loop allowance
num_agents = 3
num_objects = 4
num_threads = -1              # -1 dictates 'Use all physically available core threads'
parallel_run = false          # Global parallelization toggling
seed = -1389484284            # Deterministic RNG seed
verbose = false               # Keep terminal logs sparse
```

### 3. Experiments Executable (`mcts_experiments`)

The separate `mcts_experiments` executable allows you to run multiple MCTS instances over a range of parameters and export the results as JSON files. It reads the `[experiments]` section from your `config.toml`.

Build it with:
```bash
cmake --build build --config Release --target mcts_experiments
```

Run it:
```bash
./build/mcts_experiments
```

Example `[experiments]` configuration in `config.toml`:
```toml
[experiments]
num_agents_min = 3
num_agents_max = 3
num_objects_min = 4
num_objects_max = 4
seed_min = 42
seed_max = 42
ratio_random_min = 1.0
ratio_random_max = 1.0
ratio_random_step = 1.0
iterations = 100
verbose = false
output_directory = "results"
```

## Concept

The project models a simple allocation problem:
- there are a fixed number of agents,
- there are a fixed number of objects,
- each agent has a preference score for each object,
- the algorithm searches for an allocation that maximizes the overall score.

Instead of enumerating every possible allocation directly, the application uses Monte Carlo Tree Search:
1. Starts from an empty or partial allocation.
2. Expands the search tree by exploring candidate decisions.
3. Simulates completions of the current partial solution.
4. Propagates the obtained score back through the tree.
5. After a given number of iterations, returns the best allocation found.

## Repository Layout

- `src/`: Application entry points (`main.cpp`) and MCTS implementations (`MCTS.cpp`, etc.).
- `include/`: Public headers, metrics (`Utility.hpp`), and third-party libraries (CLI11).
- `config.toml`: Default runtime configuration file.
- `CMakeLists.txt`: Build target definitions.
- `data/`: Generated or example data.
- `results/`: Output directory for experiments.
