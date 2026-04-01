# MCTS-Allocator

**MCTS-Allocator** is a fast, C++17 engine implementing Monte Carlo Tree Search (MCTS) to solve complex **resource allocation** problems. It aims to optimally allocate a set of objects to multiple agents based on their preferences in order to maximize a global utility score (e.g., fairness or total value).

## Key Features

- **Monte Carlo Tree Search (MCTS)**: Employs standard UCB1 for tree traversal and selection, supporting customizable exploration parameters.
- **High-Performance SIMD Processing**: Leverages AVX/SSE2 (`__m128d`) intrinsics to accelerate UCB1 calculations across multiple children simultaneously.
- **Multi-Threading Capability**: Native implementation supporting `OpenMP` to parallelize simulation steps, making the most of multi-core CPUs.
- **Dual Simulation Approaches (Rollout)**: Allows swapping between purely random stochastic simulations and heuristic-guided UCB1 rollout allocations using a dynamic adjustable ratio (`--ratio-random-simulation`).
- **Dual Configuration Interfaces**:
  - **`config.toml`**: Easily repeatable and trackable parameters loaded directly from a TOML file.
  - **Command-Line Interface (CLI)**: A robust CLI powered by `CLI11` allowing you to temporarily override settings directly from the terminal.
- **Beautiful Console Reporting**: Integrates real-time, terminal-friendly progress bars (`indicators` library).

## Prerequisites

- **C++17 Compatible Compiler**: GCC, Clang, or MSVC (with `/utf-8` flag).
- **CMake**: Version 3.14 or strictly higher.
- **OpenMP** (Optional but Recommended): Automatically discovered via CMake if available on your system environment.
- **LLVM**: For modern MSVC setups aiming to use OpenMP, the LLVM OpenMP runtime is explicitly queried.

## Building the Project

1. Clone the repository to your local machine:
   ```bash
   git clone https://github.com/kaaniiel/MCTS-Allocator.git
   cd MCTS-Allocator
   ```

2. Create a build directory:
   ```bash
   mkdir build && cd build
   ```

3. Generate build files and compile using CMake:
   ```bash
   cmake ..
   cmake --build . --config Release
   ```
   > *Note: By default CMake will try to locate OpenMP. If it's found, multi-threading capabilities will automatically be linked into `mcts_core`.*

## Usage & Configuration

You can run the executable generated in the build directory.

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