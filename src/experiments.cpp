#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "config/CLI11.hpp"
#include "config/config.hpp"
#include "indicators/progress_bar.hpp"
#include "mcts/Allocation.hpp"
#include "mcts/MCTS.hpp"
#include "mcts/Score.hpp"
#include "Solver.hpp"
#include "metrics/Metrics.hpp"

namespace
{
    struct ExperimentParams
    {
        int numAgents;
        int numObjects;
        int seed;
        double ratioRandom;
        int budget;
    };

    struct SolverKey
    {
        int numAgents;
        int numObjects;
        int seed;

        bool operator==(const SolverKey &other) const
        {
            return numAgents == other.numAgents && numObjects == other.numObjects && seed == other.seed;
        }
    };

    struct SolverKeyHash
    {
        std::size_t operator()(const SolverKey &key) const
        {
            std::size_t h1 = std::hash<int>{}(key.numAgents);
            std::size_t h2 = std::hash<int>{}(key.numObjects);
            std::size_t h3 = std::hash<int>{}(key.seed);
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };

    template <typename T>
    struct SolverResultCache
    {
        bool ok;
        double score;
        std::vector<int> allocation;
        long long timeUs{0};
        Preferences<T> prefs;
    };

    /**
            if (config.enableMetrics)
            {
                add_metrics(out, config.enableMetrics, cacheIt->second.prefs, cacheIt->second.allocation, 10);
            }
            else
            {
                out << "\n";
            }
        }
        else
        {
            out << "          \"status\": \"error\",\n";
            out << "          \"score\": 0,\n";
            out << "          \"allocation\": []\n";
        }
     */
    bool is_near_integer(double value, double eps = 1e-9)
    {
        return std::fabs(value - std::round(value)) <= eps;
    }

    /**
     * @brief Format a double as a string with sufficient precision for
     * JSON/TOML output and logging.
     *
     * @param value The double to format.
     * @return A string representation of the value.
     */
    std::string format_double(double value)
    {
        std::ostringstream oss;
        oss << std::setprecision(12) << value;
        return oss.str();
    }

    /**
     * @brief Serialize a score value for JSON output.
     *
     * Returns the numeric string when finite, or the literal "null"
     * when the value is infinite or NaN.
     *
     * @param value Score to format.
     * @return String suitable for embedding in JSON.
     */
    std::string format_json_score(double value)
    {
        if (std::isfinite(value))
        {
            return format_double(value);
        }
        return "null";
    }

    /**
     * @brief Write a flat JSON-style array to the provided stream.
     *
     * This helper is used when emitting preference vectors and
     * allocations into per-experiment JSON output files.
     *
     * @tparam T Element type supporting stream output.
     * @param out    Output stream to write to.
     * @param values Vector of values to serialize as an array.
     */
    template <typename T>
    void write_array(std::ostream &out, const std::vector<T> &values)
    {
        out << "[";
        for (std::size_t i = 0; i < values.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << values[i];
        }
        out << "]";
    }

    /**
     * @brief Build a sequence of ratio values between min and max.
     *
     * Two behaviours are supported:
     * - If `step` > 1 and is an integer, it is treated as the number of
     *   evenly-spaced samples to generate between `minValue` and `maxValue`.
     * - Otherwise, `step` is treated as an increment and values are generated
     *   by stepping from `minValue` to `maxValue`.
     *
     * @param minValue Lower bound of the range.
     * @param maxValue Upper bound of the range.
     * @param step     Step increment or sample count.
     * @return Vector of ratio values.
     */
    std::vector<double> build_ratio_values(double minValue, double maxValue, double step)
    {
        constexpr double kEps = 1e-9;
        std::vector<double> values;
        if (std::fabs(minValue - maxValue) <= kEps)
        {
            values.push_back(minValue);
            return values;
        }

        // For ratio_random in [0, 1], a value > 1 is interpreted as
        // "number of samples" to generate between min and max (inclusive).
        if (step > 1.0)
        {
            if (!is_near_integer(step))
            {
                throw std::runtime_error("ratio_random_step > 1 must be an integer (used as number of ratio_random samples).");
            }

            const int sampleCount = static_cast<int>(std::llround(step));
            if (sampleCount < 2)
            {
                throw std::runtime_error("ratio_random_step interpreted as sample count must be >= 2.");
            }

            values.reserve(static_cast<std::size_t>(sampleCount));
            const double range = maxValue - minValue;
            for (int i = 0; i < sampleCount; ++i)
            {
                const double t = static_cast<double>(i) / static_cast<double>(sampleCount - 1);
                values.push_back(minValue + (t * range));
            }
            return values;
        }

        for (double current = minValue; current <= maxValue + kEps; current += step)
        {
            values.push_back(std::min(current, maxValue));
            if (values.size() > 100000)
            {
                throw std::runtime_error("Too many ratio_random values generated. Reduce [ratio_random_min, ratio_random_max] or increase ratio_random_step.");
            }
        }
        return values;
    }

    /**
     * @brief Build a list of integer values between two bounds using one or more step sizes.
     *
     * If `steps` contains a single element it is used as a fixed increment.
     * If multiple steps are provided they are applied cyclically until the
     * `maxValue` is reached or surpassed.
     *
     * @param minValue Inclusive lower bound.
     * @param maxValue Inclusive upper bound.
     * @param steps    Sequence of positive step sizes.
     * @return Vector of integer sample points covering the range.
     */
    std::vector<int> build_values_with_steps(int minValue, int maxValue, const std::vector<int> &steps)
    {
        std::vector<int> values;
        if (minValue > maxValue)
            return values;

        if (minValue == maxValue)
        {
            values.push_back(minValue);
            return values;
        }

        if (steps.empty())
        {
            // default to step of 1
            for (int v = minValue; v <= maxValue; ++v)
                values.push_back(v);
            return values;
        }

        // Validate positive steps
        for (int s : steps)
        {
            if (s <= 0)
                throw std::runtime_error("Step values must be positive integers.");
        }

        if (steps.size() == 1)
        {
            int step = steps[0];
            for (int v = minValue; v <= maxValue; v += step)
                values.push_back(v);
            // ensure last element equals maxValue
            if (values.empty() || values.back() != maxValue)
                values.push_back(maxValue);
            return values;
        }

        // Multiple-step sequence: apply cyclically
        int cur = minValue;
        values.push_back(cur);
        std::size_t idx = 0;
        while (true)
        {
            cur += steps[idx];
            if (cur > maxValue)
                break;
            values.push_back(cur);
            idx = (idx + 1) % steps.size();
        }
        if (values.empty() || values.back() != maxValue)
            values.push_back(maxValue);
        return values;
    }

    /**
     * @brief Construct the full grid of experiments from the configuration.
     *
     * This expands ranges for agents, objects, seeds and ratioRandom into
     * an explicit list of `ExperimentParams` describing each experiment
     * instance to run.
     *
     * @param config Parsed configuration containing range parameters.
     * @return Vector of `ExperimentParams` describing all experiments.
     */
    std::vector<ExperimentParams> build_experiment_grid(const Config &config)
    {
        const std::vector<double> ratioValues = build_ratio_values(config.ratioRandomMin, config.ratioRandomMax, config.ratioRandomStep);
        std::vector<ExperimentParams> experiments;

        const std::vector<int> agentValues = build_values_with_steps(config.numAgentsMin, config.numAgentsMax, config.stepAgents);
        const std::vector<int> objectValues = build_values_with_steps(config.numObjectsMin, config.numObjectsMax, config.stepObjects);
        const std::vector<int> seedValues = build_values_with_steps(config.seedMin, config.seedMax, config.stepSeeds);

        for (int numAgents : agentValues)
        {
            for (int numObjects : objectValues)
            {
                // Skip configurations where there are fewer objects than agents.
                if (numObjects < numAgents)
                    continue;
                for (int seed : seedValues)
                {
                    for (double ratioRandom : ratioValues)
                    {
                        const int dynamicBudget = numAgents * numObjects;
                        experiments.push_back(ExperimentParams{numAgents, numObjects, seed, ratioRandom, dynamicBudget});
                    }
                }
            }
        }

        return experiments;
    }

    /**
     * @brief Compute cumulative budget targets for each step.
     *
     * Distributes `budget` across `numberOfSteps` steps as evenly as
     * possible, returning a vector of cumulative targets for each step.
     *
     * @param budget         Total budget to distribute.
     * @param numberOfSteps  Number of steps to divide the budget into.
     * @return Vector of cumulative budget targets per step.
     */
    std::vector<int> build_step_targets(int budget, int numberOfSteps)
    {
        if (numberOfSteps <= 0)
        {
            return {budget};
        }

        if (budget <= 0)
        {
            return std::vector<int>(static_cast<std::size_t>(numberOfSteps), 0);
        }

        std::vector<int> targets;
        targets.reserve(static_cast<std::size_t>(numberOfSteps));

        const int baseIncrement = budget / numberOfSteps;
        const int remainder = budget % numberOfSteps;

        int cumulative = 0;
        for (int step = 1; step <= numberOfSteps; ++step)
        {
            const int increment = baseIncrement + (step <= remainder ? 1 : 0);
            cumulative = std::min(budget, cumulative + increment);
            targets.push_back(cumulative);
        }

        if (!targets.empty())
        {
            targets.back() = budget;
        }

        return targets;
    }

    /**
     * @brief Build a timestamped filename for the experiment run.
     *
     * The produced filename has the form `experiments_DD-MM-YYYY_HH-MM-SS.json`.
     *
     * @return Filename string (not including output directory).
     */
    std::string build_output_filename()
    {
        auto t = std::time(nullptr);
        std::tm tm{};
#if defined(_WIN32) || defined(_WIN64)
        localtime_s(&tm, &t);
#else
        localtime_r(&t, &tm);
#endif
        std::ostringstream oss;
        oss << "experiments_" << std::put_time(&tm, "%d-%m-%Y_%H-%M-%S") << ".json";
        return oss.str();
    }

    /**
     * @brief Create and configure a progress bar used during experiment runs.
     *
     * The progress bar is preconfigured with visual options used by this
     * program (width, colors, prefix/postfix text, etc.).
     *
     * @return A configured `indicators::ProgressBar` instance.
     */
    indicators::ProgressBar create_progress_bar()
    {
        return indicators::ProgressBar{
            indicators::option::BarWidth{60},
            indicators::option::Start{"|"},
            indicators::option::End{"|"},
            indicators::option::Fill{"="},
            indicators::option::Lead{">"},
            indicators::option::Remainder{"."},
            indicators::option::PrefixText{"MCTS experiments"},
            indicators::option::PostfixText{"Running experiments..."},
            indicators::option::ForegroundColor{indicators::Color::cyan},
            indicators::option::ShowPercentage{true},
            indicators::option::ShowElapsedTime{true},
            indicators::option::ShowRemainingTime{true},
            indicators::option::FontStyles{std::vector<indicators::FontStyle>{indicators::FontStyle::bold}}};
    }

    /**
     * @brief Update a progress bar based on work completed and total units.
     *
     * Safely handles the case `total == 0`.
     *
     * @param bar   Progress bar to update.
     * @param done  Number of completed work units.
     * @param total Total work units.
     */
    void update_progress(indicators::ProgressBar &bar, std::size_t done, std::size_t total)
    {
        if (total == 0)
        {
            bar.set_progress(100);
            return;
        }
        const std::size_t progress = static_cast<std::size_t>((100.0 * static_cast<double>(done)) / static_cast<double>(total));
        bar.set_progress(progress);
    }

    /**
     * @brief Format a duration given in microseconds into a human-readable string.
     *
     * The output includes hours, minutes, seconds, milliseconds and microseconds
     * as appropriate.
     *
     * @param totalUs Duration in microseconds.
     * @return Formatted duration string (e.g. "1h 2m 3s 4ms 500us").
     */
    std::string format_duration_us(long long totalUs)
    {
        const auto totalMs = totalUs / 1000;
        const auto hours = totalMs / 3600000;
        const auto minutes = (totalMs % 3600000) / 60000;
        const auto seconds = (totalMs % 60000) / 1000;
        const auto milliseconds = totalMs % 1000;
        const auto microseconds = totalUs % 1000;

        std::ostringstream oss;
        if (hours > 0)
        {
            oss << hours << "h ";
        }
        if (hours > 0 || minutes > 0)
        {
            oss << minutes << "m ";
        }
        oss << seconds << "s " << milliseconds << "ms " << microseconds << "us";
        return oss.str();
    }

    /**
     * @brief Throttle status updates to a minimum time interval.
     *
     * Returns true and updates `lastStatus` when at least `minInterval`
     * has elapsed since the previous update.
     *
     * @param lastStatus  Reference to the previous status timestamp.
     * @param minInterval Minimum interval between status emissions.
     * @return true if a status should be emitted now, false otherwise.
     */
    bool should_emit_status(std::chrono::steady_clock::time_point &lastStatus,
                            std::chrono::milliseconds minInterval = std::chrono::milliseconds(2000))
    {
        const auto now = std::chrono::steady_clock::now();
        if ((now - lastStatus) >= minInterval)
        {
            lastStatus = now;
            return true;
        }
        return false;
    }

    /**
     * @brief Build a human-readable postfix string displayed in the live progress bar.
     *
     * Contains experiment/try/step indices, budget progress and configuration
     * identifiers (agents, objects, seed, ratioRandom).
     *
     * @return Formatted postfix string for display.
     */
    std::string build_live_postfix(std::size_t expIndex,
                                   std::size_t totalExperiments,
                                   int tryIndex,
                                   int totalTries,
                                   std::size_t stepIndex,
                                   std::size_t totalSteps,
                                   int executedBudget,
                                   int targetBudget,
                                   int numAgents,
                                   int numObjects,
                                   int seed,
                                   double ratioRandom)
    {
        std::ostringstream oss;
        oss << "exp " << (expIndex + 1) << "/" << totalExperiments
            << " try " << tryIndex << "/" << totalTries
            << " step " << (stepIndex + 1) << "/" << totalSteps
            // << " budget " << executedBudget << "->" << targetBudget
            << " a=" << numAgents
            << " o=" << numObjects
            // << " s=" << seed
            // << " r=" << format_double(ratioRandom)
            ;
        return oss.str();
    }

    /**
     * @brief Append fairness/utility metrics to the experiment JSON output.
     *
     * If `enableMetrics` is false the function is a no-op. When enabled the
     * function writes a JSON object with metric boolean results (Prop, EF, EFX, EF1).
     *
     * @param out             Output stream to append to.
     * @param enableMetrics   Whether to compute and emit metrics.
     * @param prefs           Preferences used for metric computation.
     * @param alloc           Allocation vector to evaluate.
     * @param numberWhiteSpace How many leading spaces to emit for formatting.
     */
    void add_metrics(std::ostream &out, bool enableMetrics, const Preferences<int> &prefs, const std::vector<int> &alloc, const int numberWhiteSpace)
    {
        if (!enableMetrics)
        {
            return;
        }

        using MetricFunc = bool (*)(const Preferences<int> &, const std::vector<int> &);

        struct Metric
        {
            const char *name;
            MetricFunc func;
        };

        static const std::vector<Metric> metrics = {
            {"ParetoOptimal", isParetoOptimal<int>},
            {"Prop", isProp<int>},
            {"EF", isEF<int>},
            {"EFX", isEFX<int>},
            {"EF1", isEF1<int>}};

        const std::string whiteSpaceStr(static_cast<std::size_t>(numberWhiteSpace), ' ');

        out << ",\n";
        out << whiteSpaceStr << "\"metrics\": {\n";
        for (std::size_t i = 0; i < metrics.size(); ++i)
        {
            const bool result = metrics[i].func(prefs, alloc);
            out << whiteSpaceStr << "  \"" << metrics[i].name << "\": " << (result ? "true" : "false");
            if (i + 1 < metrics.size())
            {
                out << ",";
            }
            out << "\n";
        }
        out << whiteSpaceStr << "}\n";
    }

    /**
     * @brief Uniformize negative score values to a consistent value if configured.
     *
     * When `config.uniformizeNegativeValues` is true, any negative score value is replaced with the negative count of agents that did not receive any object. This provides a more interpretable and
     * consistent way to handle negative scores, which may arise from certain preference structures or scoring functions. When the configuration option is false, the original score value is returned unchanged.
     *
     * @param alloc The allocation being evaluated, used to count agents without objects.
     * @param value The original score value to potentially uniformize.
     * @param config The experiment configuration containing the `uniformizeNegativeValues` option.
     * @return The uniformized score value if negative and uniformization is enabled, otherwise the original value.
     */
    template <typename T>
    T uniformize_negative_values(const int numAgents, const std::vector<int> &alloc, T value, const Config &config)
    {
        if (value < 0 && config.uniformizeNegativeValues)
        {
            // Return the number of agents that haven't received an object
            int agentsWithoutObject = numAgents;
            std::vector<bool> agentHasObject(alloc.size(), false);
            for (int object = 0; object < alloc.size(); ++object)
            {
                int allocatedAgent = alloc[object];
                if (agentHasObject[allocatedAgent])
                {
                    continue;
                }

                if (allocatedAgent >= 0 && allocatedAgent < alloc.size())
                {
                    agentHasObject[allocatedAgent] = true;
                    agentsWithoutObject--;
                }
            }
            return -static_cast<T>(agentsWithoutObject);
        }
        return value;
    }
}
int main(int argc, char **argv)
{
    const auto programStart = std::chrono::steady_clock::now();

    std::string configPath = "config.toml";
    std::string resumeDir = "";

    CLI::App app{"MCTS experiments runner"};
    app.add_option("-c,--config", configPath, "Path to TOML configuration file");
    app.add_option("-r,--resume", resumeDir, "Path to a previous run directory to resume from (optional)");
    CLI11_PARSE(app, argc, argv);

    if (!std::filesystem::exists(configPath))
    {
        const std::filesystem::path cfgPath(configPath);
        const std::filesystem::path parentDir = cfgPath.parent_path();
        if (!parentDir.empty())
        {
            std::filesystem::create_directories(parentDir);
        }

        Config defaultConfig;
        Config::generate_default(configPath, defaultConfig);
        std::cerr << "[Config] Missing configuration file: " << configPath << "\n";
        std::cerr << "[Config] A default configuration file has been generated. Please review it, then re-run the experiments.\n";
        return EXIT_FAILURE;
    }

    Config config;
    try
    {
        config = Config::load(configPath);
    }
    catch (const std::exception &ex)
    {
        std::cerr << ex.what() << "\n";
        return EXIT_FAILURE;
    }

    std::vector<ExperimentParams> experimentGrid;
    try
    {
        experimentGrid = build_experiment_grid(config);
    }
    catch (const std::exception &ex)
    {
        std::cerr << "[Config] " << ex.what() << "\n";
        return EXIT_FAILURE;
    }

    const int numberOfSteps = static_cast<int>(std::llround(config.numberOfBudgetStep));
    const std::size_t stepsPerTry = static_cast<std::size_t>(numberOfSteps > 0 ? numberOfSteps : 1);

    std::unordered_set<SolverKey, SolverKeyHash> uniqueSolverKeys;
    uniqueSolverKeys.reserve(experimentGrid.size());
    for (const ExperimentParams &params : experimentGrid)
    {
        uniqueSolverKeys.insert(SolverKey{params.numAgents, params.numObjects, params.seed});
    }

    const std::size_t solverWorkUnits = uniqueSolverKeys.size();
    const std::size_t mctsWorkUnits = experimentGrid.size() * static_cast<std::size_t>(config.numberOfTrys) * stepsPerTry;
    const std::size_t totalWorkUnits = solverWorkUnits + mctsWorkUnits;
    std::size_t completedWorkUnits = 0;
    auto progressBar = create_progress_bar();
    auto lastLiveStatus = std::chrono::steady_clock::now();
    std::unordered_map<SolverKey, SolverResultCache<int>, SolverKeyHash> solverCache;
    solverCache.reserve(solverWorkUnits);

    std::filesystem::path runDir;
    std::size_t startExpIndex = 0;

    if (!resumeDir.empty())
    {
        runDir = resumeDir;
        if (std::filesystem::exists(runDir))
        {
            for (std::size_t i = 0; i < experimentGrid.size(); ++i)
            {
                std::ostringstream fnameStream;
                fnameStream << "experiment_" << (i + 1)
                            << "_a" << experimentGrid[i].numAgents
                            << "_o" << experimentGrid[i].numObjects
                            << "_s" << experimentGrid[i].seed
                            << ".json";

                if (std::filesystem::exists(runDir / fnameStream.str()))
                {
                    // On retient l'index du dernier fichier existant pour l'écraser
                    startExpIndex = i;
                }
                else
                {
                    break; // Premier fichier manquant trouvé
                }
            }
            std::cout << "[Checkpoint] Reprise detectee. Ecrasement et reprise a l'experience "
                      << (startExpIndex + 1) << "/" << experimentGrid.size() << "\n";
        }
        else
        {
            std::cerr << "[Checkpoint] Erreur : le dossier " << resumeDir << " n'existe pas.\n";
            return EXIT_FAILURE;
        }
    }
    else
    {
        std::filesystem::create_directories(config.outputDirectory);
        const std::string runBasename = std::filesystem::path(build_output_filename()).stem().string();
        runDir = std::filesystem::path(config.outputDirectory) / runBasename;
        std::filesystem::create_directories(runDir);
    }

    // Avancer artificiellement la barre de chargement pour compenser les étapes sautées
    completedWorkUnits = startExpIndex * static_cast<std::size_t>(config.numberOfTrys) * stepsPerTry;

    for (std::size_t expIndex = startExpIndex; expIndex < experimentGrid.size(); ++expIndex)
    {
        const ExperimentParams &params = experimentGrid[expIndex];

        // Build per-experiment output file inside the run directory
        std::ostringstream fnameStream;
        fnameStream << "experiment_" << (expIndex + 1)
                    << "_a" << params.numAgents
                    << "_o" << params.numObjects
                    << "_s" << params.seed
                    << ".json";
        const std::filesystem::path expPath = runDir / fnameStream.str();

        std::ofstream out(expPath);
        if (!out.is_open())
        {
            std::cerr << "[Results] Error: unable to create output file at " << expPath.string() << "\n";
            continue;
        }

        out << "{\n";

        Config runConfig = config;
        runConfig.numAgents = params.numAgents;
        runConfig.numObjects = params.numObjects;
        runConfig.seed = params.seed;
        runConfig.ratioRandom = params.ratioRandom;
        runConfig.iterations = params.budget;

        const std::vector<int> stepTargets = build_step_targets(params.budget, numberOfSteps);

        out << "      \"parameters\": {\n";
        out << "        \"numAgents\": " << params.numAgents << ",\n";
        out << "        \"numObjects\": " << params.numObjects << ",\n";
        out << "        \"seed\": " << params.seed << ",\n";
        out << "        \"ratioRandom\": " << format_double(params.ratioRandom) << ",\n";
        out << "        \"budget\": " << params.budget << "\n";
        out << "      },\n";
        out << "      \"preferences\": [\n";
        try
        {
            Preferences<int> prefs(params.numAgents, params.numObjects, false);
            prefs.generateRandomPreferences(params.numAgents * params.numObjects, params.seed);
            for (int agent = 0; agent < params.numAgents; ++agent)
            {
                out << "\t\t\t\t";
                write_array(out, prefs.getPreference(agent));
                if (agent < params.numAgents - 1)
                {
                    out << ",\n";
                }
            }
            out << "],\n";
        }
        catch (const std::exception &)
        {
            out << "        \"preferences\": []\n";
        }
        out << "      \"results\": {\n";

        out << "        \"solver\": {\n";

        const SolverKey solverKey{params.numAgents, params.numObjects, params.seed};
        auto cacheIt = solverCache.find(solverKey);
        if (cacheIt == solverCache.end())
        {
            std::ostringstream solverStatus;
            solverStatus << "solver a=" << params.numAgents
                         << " o=" << params.numObjects
                         << " s=" << params.seed;
            progressBar.set_option(indicators::option::PostfixText{solverStatus.str()});

            SolverResultCache<int> computedResult{};
            try
            {
                Solver<int> solver(runConfig);
                computedResult.prefs = solver.getPreferences();
                const auto solverStart = std::chrono::steady_clock::now();
                std::pair<Allocation, Score> solverResult = solver.solve(config.verbose);
                const auto solverEnd = std::chrono::steady_clock::now();
                computedResult.ok = true;
                computedResult.score = solverResult.second.getScore();
                computedResult.allocation = solverResult.first.getAllocation();
                computedResult.timeUs = std::chrono::duration_cast<std::chrono::microseconds>(solverEnd - solverStart).count();
            }
            catch (const std::exception &)
            {
                computedResult.ok = false;
                computedResult.score = 0.0;
                computedResult.allocation.clear();
                computedResult.timeUs = 0;
            }

            cacheIt = solverCache.emplace(solverKey, std::move(computedResult)).first;
            ++completedWorkUnits;
            update_progress(progressBar, completedWorkUnits, totalWorkUnits);
        }

        if (cacheIt->second.ok)
        {
            out << "          \"score\": " << format_json_score(cacheIt->second.score) << ",\n";
            out << "          \"timeUs\": " << cacheIt->second.timeUs << ",\n";
            // out << "          \"time\": \"" << format_duration_us(cacheIt->second.timeUs) << "\",\n";
            out << "          \"allocation\": ";
            write_array(out, cacheIt->second.allocation);
            if (config.enableMetrics)
            {
                add_metrics(out, config.enableMetrics, cacheIt->second.prefs, cacheIt->second.allocation, 10);
            }
            else
            {
                out << "\n";
            }
            out << "\n";
        }
        else
        {
            out << "          \"score\": 0,\n";
            out << "          \"allocation\":[]";
            if (config.enableMetrics)
            {
                add_metrics(out, config.enableMetrics, cacheIt->second.prefs, cacheIt->second.allocation, 10);
            }
            else
            {
                out << "\n";
            }
            out << "\n";
        }

        out << "        },\n";

        out << "        \"mcts\": {\n";
        out << "          \"tries\": [\n";

        for (int tryIndex = 1; tryIndex <= config.numberOfTrys; ++tryIndex)
        {
            std::pair<Allocation, Score> finalBest(Allocation(params.numAgents, params.numObjects), Score(0.0));

            out << "            {\n";
            out << "              \"steps\": [\n";

            // Create MCTS in a limited scope to ensure memory is freed after each try
            auto tryStart = std::chrono::steady_clock::now();
            auto lastStepTime = tryStart;
            {
                MCTS<int> mcts(runConfig);
                Preferences<int> pref = mcts.getPreferences();
                int executedBudget = 0;

                for (std::size_t stepIdx = 0; stepIdx < stepTargets.size(); ++stepIdx)
                {
                    const int targetBudget = stepTargets[stepIdx];
                    const int delta = targetBudget - executedBudget;
                    if (delta > 0)
                    {
                        const int chunkSize = std::max(1, params.budget / 20);
                        int remaining = delta;
                        while (remaining > 0)
                        {
                            const int chunk = std::min(chunkSize, remaining);
                            mcts.run(chunk, false);
                            executedBudget += chunk;
                            remaining -= chunk;

                            if (should_emit_status(lastLiveStatus))
                            {
                                progressBar.set_option(indicators::option::PostfixText{
                                    build_live_postfix(
                                        expIndex,
                                        experimentGrid.size(),
                                        tryIndex,
                                        config.numberOfTrys,
                                        stepIdx,
                                        stepTargets.size(),
                                        executedBudget,
                                        targetBudget,
                                        params.numAgents,
                                        params.numObjects,
                                        params.seed,
                                        params.ratioRandom)});
                            }
                        }
                    }

                    finalBest = mcts.getRoot().getBestAllocation();
                    const int remainingBudget = std::max(0, params.budget - targetBudget);

                    const auto stepNow = std::chrono::steady_clock::now();
                    const auto stepDurationUs = std::chrono::duration_cast<std::chrono::microseconds>(stepNow - lastStepTime).count();
                    const auto cumulativeUs = std::chrono::duration_cast<std::chrono::microseconds>(stepNow - tryStart).count();
                    lastStepTime = stepNow;

                    out << "                {\n";
                    out << "                  \"stepTimeUs\": " << stepDurationUs << ",\n";
                    out << "                  \"score\": " << format_json_score(uniformize_negative_values(finalBest.first.getNumAgents(), finalBest.first.getAllocation(), finalBest.second.getScore(), config)) << ",\n";
                    out << "                  \"allocation\": ";
                    write_array(out, finalBest.first.getAllocation());
                    if (config.enableMetrics)
                    {
                        add_metrics(out, config.enableMetrics, pref, finalBest.first.getAllocation(), 18);
                    }
                    else
                    {
                        out << "\n";
                    }
                    out << "                }";
                    if (stepIdx + 1 < stepTargets.size())
                    {
                        out << ",";
                    }
                    out << "\n";

                    ++completedWorkUnits;
                    update_progress(progressBar, completedWorkUnits, totalWorkUnits);
                }
            } // MCTS object is destroyed here, freeing all memory
            // Force memory cleanup
            std::cout.flush();

            const auto tryEnd = std::chrono::steady_clock::now();
            const auto tryDurationUs = std::chrono::duration_cast<std::chrono::microseconds>(tryEnd - tryStart).count();

            out << "              ]\n";

            out << "\n";
            out << "            }";
            if (tryIndex < config.numberOfTrys)
            {
                out << ",";
            }
            out << "\n";
        }

        out << "          ]\n";
        out << "        }\n";
        out << "      }\n";
        out << "}\n";
        out.close();
    }

    (void)0; // all outputs written per-experiment in run directory

    progressBar.set_option(indicators::option::PostfixText{"Running experiments..."});

    const auto programEnd = std::chrono::steady_clock::now();
    const auto totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(programEnd - programStart);

    std::cout << "[Experiments] Completed " << experimentGrid.size() << " experiment parameter set(s).\n";
    std::cout << "[Experiments] Results written to " << runDir.string() << "\n";
    std::cout << "[Experiments] Total runtime: " << format_duration_us(std::chrono::duration_cast<std::chrono::microseconds>(totalDuration).count()) << "\n";

    return EXIT_SUCCESS;
}
