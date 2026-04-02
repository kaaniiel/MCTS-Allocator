# MCTS Allocator

MCTS Allocator is a C++17 project that explores resource allocation with a Monte Carlo Tree Search (MCTS) approach. The program builds a search tree of possible allocations, evaluates candidate solutions with a configurable scoring function, and reports the best allocation it finds for a given problem size.

This repository is intentionally focused on the core idea of the algorithm and on how to run it in a portable way with CMake. It does not aim to document advanced tuning or performance workarounds.

## Quick Start

If you just want to build and run the project for the first time, the shortest path is:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

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
cmake --build build --config Release
```

On single-config generators, the `--config Release` part is harmless. On multi-config generators, it is required to choose the Release configuration.

### Step 4: Run the executable

The main binary is named `mcts_main`.

- On Linux and macOS, it will usually be located in `build/` or a generator-specific subdirectory.
- On Windows with Visual Studio generators, it is commonly placed under a `Release/` folder inside the build tree.

Example:

```bash
./build/mcts_main
```

If your generator places the executable in a configuration subfolder, run the binary from that location instead.

## Using VS Code

If you use the CMake Tools extension in VS Code, the workflow is the same:

1. Open the folder in VS Code.
2. Select a CMake kit or compiler toolchain.
3. Choose the Release variant when the extension asks for a build configuration.
4. Run Configure.
5. Run Build.

For multi-config generators, make sure the active configuration shown by CMake Tools is Release before building.

## Runtime Configuration

The application reads its default settings from `config.toml`.

If the file does not exist, the program generates one automatically with default values.

### Default settings

The most relevant settings are:

- `num_agents`: number of agents in the allocation problem
- `num_objects`: number of objects to allocate
- `iterations`: number of MCTS iterations to run
- `exploration_constant`: exploration term used by the tree policy
- `seed`: seed used for preference generation
- `verbose`: enables additional diagnostic output
- `ratio_random`: controls the mix between random and heuristic simulations
- `save_results`: enables JSON export of the final result
- `launch`: switches the application into the graph/export mode instead of running the search directly

Example configuration:

```toml
[mcts]
exploration_constant = 1.414
iterations = 100
num_agents = 3
num_objects = 4
ratio_random = 1.0
save_results = false
seed = 42
verbose = false
```

## Command-Line Arguments

The executable also accepts command-line overrides. Command-line values take priority over the TOML file.

| Option               | Description                              |
| -------------------- | ---------------------------------------- |
| `-n, --num-agents`   | Override the number of agents            |
| `-o, --num-objects`  | Override the number of objects           |
| `-i, --iterations`   | Override the number of search iterations |
| `-e, --exploration`  | Override the exploration constant        |
| `-s, --seed`         | Override the random seed                 |
| `-l, --launch`       | Launch the graph/export mode             |
| `-v, --verbose`      | Enable verbose output                    |
| `-r, --ratio-random` | Override the ratio of random simulations |
| `-S, --save-results` | Save the final result as JSON            |

Example:

```bash
./build/mcts_main --num-agents 5 --num-objects 8 --iterations 250 --save-results
```

## What The Program Does At Runtime

When the application starts, it:

1. Loads `config.toml` or creates it with default values.
2. Applies command-line overrides.
3. Generates preferences for the configured number of agents and objects.
4. Runs the MCTS search for the requested number of iterations.
5. Prints the best allocation and its score.
6. Optionally writes a JSON result file in a `results/` directory.

If `--launch` is enabled, the application follows a graph/export path instead of running the search loop.

## Output

By default, the program prints:

- the resolved configuration,
- the best allocation found,
- the score associated with that allocation.

If result export is enabled, the application creates a file named like `mcts_resultsYYYY-MM-DD_HH-MM-SS.json` inside `results/`.

## Example Workflow

### Linux or macOS

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
./build/mcts_main --iterations 200 --save-results
```

### Windows PowerShell

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
.\build\Release\mcts_main.exe --iterations 200 --save-results
```

If you use Visual Studio as the generator, the executable is usually under `build/Release/`. If you use Ninja, it is typically under `build/`.

## Notes

- The repository ships with a default `config.toml`, but the program can regenerate one if needed.
- The algorithm focuses on the allocation principle and on building an understandable search workflow.
- The code is set up to be portable through CMake, so the same build steps apply across platforms with only minor path differences.

## License

See [LICENSE](LICENSE) for the license terms.
