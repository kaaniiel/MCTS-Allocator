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

    struct SolverResultCache
    {
        bool ok;
        double score;
        std::vector<int> allocation;
    };

    bool is_near_integer(double value, double eps = 1e-9)
    {
        return std::fabs(value - std::round(value)) <= eps;
    }

    std::string format_double(double value)
    {
        std::ostringstream oss;
        oss << std::setprecision(12) << value;
        return oss.str();
    }

    void write_int_array(std::ostream &out, const std::vector<int> &values)
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

    std::vector<std::string> validate_experiment_config(const Config &config)
    {
        std::vector<std::string> errors;

        auto push_error = [&errors](const std::string &message)
        {
            errors.push_back(message);
        };

        if (config.numAgentsMin <= 0)
            push_error("experiments.num_agents_min must be > 0.");
        if (config.numAgentsMax <= 0)
            push_error("experiments.num_agents_max must be > 0.");
        if (config.numAgentsMin > config.numAgentsMax)
            push_error("experiments.num_agents_min must be <= experiments.num_agents_max.");

        if (config.numObjectsMin <= 0)
            push_error("experiments.num_objects_min must be > 0.");
        if (config.numObjectsMax <= 0)
            push_error("experiments.num_objects_max must be > 0.");
        if (config.numObjectsMin > config.numObjectsMax)
            push_error("experiments.num_objects_min must be <= experiments.num_objects_max.");
        int budget = config.numObjectsMax * config.numAgentsMax;

        if (config.seedMin > config.seedMax)
            push_error("experiments.seed_min must be <= experiments.seed_max.");

        if (config.ratioRandomMin < 0.0 || config.ratioRandomMin > 1.0)
            push_error("experiments.ratio_random_min must be in [0, 1].");
        if (config.ratioRandomMax < 0.0 || config.ratioRandomMax > 1.0)
            push_error("experiments.ratio_random_max must be in [0, 1].");
        if (config.ratioRandomMin > config.ratioRandomMax)
            push_error("experiments.ratio_random_min must be <= experiments.ratio_random_max.");
        if (config.ratioRandomStep <= 0.0 && std::fabs(config.ratioRandomMin - config.ratioRandomMax) > 1e-9)
            push_error("experiments.ratio_random_step must be > 0 when ratio_random_min != ratio_random_max.");
        if (config.ratioRandomStep > 1.0 && !is_near_integer(config.ratioRandomStep))
            push_error("experiments.ratio_random_step > 1 is interpreted as sample count and must be an integer.");

        if (config.numberOfTrys <= 0)
            push_error("experiments.number_of_trys must be > 0.");

        if (config.numberOfBudgetStep < 0.0)
            push_error("experiments.numberOfBudgetStep must be >= 0.");
        if (!is_near_integer(config.numberOfBudgetStep))
            push_error("experiments.numberOfBudgetStep must be an integer value (0, 1, 2, ...).");

        if (config.numberOfBudgetStep > 0.0)
        {
            const int numberOfSteps = static_cast<int>(std::round(config.numberOfBudgetStep));
            const int minDynamicBudget = config.numAgentsMin * config.numObjectsMin;
            if (numberOfSteps > minDynamicBudget)
                push_error("experiments.numberOfBudgetStep cannot be greater than the dynamic budget (numAgents * numObjects) of any experiment.");
        }

        if (config.outputDirectory.empty())
            push_error("experiments.output_directory cannot be empty.");

        return errors;
    }

    std::vector<ExperimentParams> build_experiment_grid(const Config &config)
    {
        const std::vector<double> ratioValues = build_ratio_values(config.ratioRandomMin, config.ratioRandomMax, config.ratioRandomStep);
        std::vector<ExperimentParams> experiments;

        for (int numAgents = config.numAgentsMin; numAgents <= config.numAgentsMax; ++numAgents)
        {
            for (int numObjects = config.numObjectsMin; numObjects <= config.numObjectsMax; ++numObjects)
            {
                for (int seed = config.seedMin; seed <= config.seedMax; ++seed)
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

    std::vector<int> build_step_targets(int budget, int numberOfSteps)
    {
        if (numberOfSteps <= 0)
        {
            return {budget};
        }

        std::vector<int> targets;
        targets.reserve(static_cast<std::size_t>(numberOfSteps));

        int previous = 0;
        for (int step = 1; step <= numberOfSteps; ++step)
        {
            int target = static_cast<int>(std::llround((static_cast<double>(step) * static_cast<double>(budget)) / static_cast<double>(numberOfSteps)));
            target = std::max(target, previous + 1);
            target = std::min(target, budget);
            targets.push_back(target);
            previous = target;
        }

        if (!targets.empty())
        {
            targets.back() = budget;
        }

        return targets;
    }

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

    std::string format_duration(std::chrono::milliseconds duration)
    {
        const auto totalMs = duration.count();
        const auto hours = totalMs / 3600000;
        const auto minutes = (totalMs % 3600000) / 60000;
        const auto seconds = (totalMs % 60000) / 1000;
        const auto milliseconds = totalMs % 1000;

        std::ostringstream oss;
        if (hours > 0)
        {
            oss << hours << "h ";
        }
        if (hours > 0 || minutes > 0)
        {
            oss << minutes << "m ";
        }
        oss << seconds << "s " << milliseconds << "ms";
        return oss.str();
    }

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
            << " budget " << executedBudget << "->" << targetBudget
            << " a=" << numAgents
            << " o=" << numObjects
            << " s=" << seed
            << " r=" << format_double(ratioRandom);
        return oss.str();
    }
} // namespace

int main(int argc, char **argv)
{
    const auto programStart = std::chrono::steady_clock::now();

    std::string configPath = "config.toml";

    CLI::App app{"MCTS experiments runner"};
    app.add_option("-c,--config", configPath, "Path to TOML configuration file");

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

    const std::vector<std::string> errors = validate_experiment_config(config);
    if (!errors.empty())
    {
        std::cerr << "[Config] Invalid experiments configuration. Startup cancelled.\n";
        for (const std::string &error : errors)
        {
            std::cerr << " - " << error << "\n";
        }
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
    std::unordered_map<SolverKey, SolverResultCache, SolverKeyHash> solverCache;
    solverCache.reserve(solverWorkUnits);

    std::filesystem::create_directories(config.outputDirectory);
    const std::filesystem::path outputPath = std::filesystem::path(config.outputDirectory) / build_output_filename();

    std::ofstream out(outputPath);
    if (!out.is_open())
    {
        std::cerr << "[Results] Error: unable to create output file at " << outputPath.string() << "\n";
        return EXIT_FAILURE;
    }

    out << "{\n";
    out << "  \"experiments\": [\n";

    for (std::size_t expIndex = 0; expIndex < experimentGrid.size(); ++expIndex)
    {
        const ExperimentParams &params = experimentGrid[expIndex];

        Config runConfig = config;
        runConfig.numAgents = params.numAgents;
        runConfig.numObjects = params.numObjects;
        runConfig.seed = params.seed;
        runConfig.ratioRandom = params.ratioRandom;
        runConfig.iterations = params.budget;

        const std::vector<int> stepTargets = build_step_targets(params.budget, numberOfSteps);

        out << "    {\n";
        out << "      \"parameters\": {\n";
        out << "        \"numAgents\": " << params.numAgents << ",\n";
        out << "        \"numObjects\": " << params.numObjects << ",\n";
        out << "        \"seed\": " << params.seed << ",\n";
        out << "        \"ratioRandom\": " << format_double(params.ratioRandom) << ",\n";
        out << "        \"budget\": " << params.budget << "\n";
        out << "      },\n";
        out << "      \"results\": {\n";

        out << "        \"solver\": {\n";
        out << "          \"name\": \"Gurobi\",\n";

        const SolverKey solverKey{params.numAgents, params.numObjects, params.seed};
        auto cacheIt = solverCache.find(solverKey);
        if (cacheIt == solverCache.end())
        {
            std::ostringstream solverStatus;
            solverStatus << "solver a=" << params.numAgents
                         << " o=" << params.numObjects
                         << " s=" << params.seed;
            progressBar.set_option(indicators::option::PostfixText{solverStatus.str()});

            SolverResultCache computedResult{};
            try
            {
                Solver<int> solver(runConfig);
                std::pair<Allocation, Score> solverResult = solver.solve(config.verbose);
                computedResult.ok = true;
                computedResult.score = solverResult.second.getScore();
                computedResult.allocation = solverResult.first.getAllocation();
            }
            catch (const std::exception &)
            {
                computedResult.ok = false;
                computedResult.score = 0.0;
                computedResult.allocation.clear();
            }

            cacheIt = solverCache.emplace(solverKey, std::move(computedResult)).first;
            ++completedWorkUnits;
            update_progress(progressBar, completedWorkUnits, totalWorkUnits);
        }

        if (cacheIt->second.ok)
        {
            out << "          \"status\": \"ok\",\n";
            out << "          \"score\": " << format_double(cacheIt->second.score) << ",\n";
            out << "          \"allocation\": ";
            write_int_array(out, cacheIt->second.allocation);
            out << "\n";
        }
        else
        {
            out << "          \"status\": \"error\",\n";
            out << "          \"score\": 0,\n";
            out << "          \"allocation\": []\n";
        }

        out << "        },\n";

        out << "        \"mcts\": {\n";
        out << "          \"tries\": [\n";

        for (int tryIndex = 1; tryIndex <= config.numberOfTrys; ++tryIndex)
        {
            MCTS<int> mcts(runConfig);
            int executedBudget = 0;
            std::pair<Allocation, Score> finalBest(Allocation(params.numAgents, params.numObjects), Score(0.0));

            out << "            {\n";
            out << "              \"tryIndex\": " << tryIndex << ",\n";
            out << "              \"steps\": [\n";

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
                const double percentBudgetUsed = static_cast<double>(targetBudget) / static_cast<double>(params.budget);

                out << "                {\n";
                out << "                  \"currentBudget\": " << targetBudget << ",\n";
                out << "                  \"percentBudgetUsed\": " << format_double(percentBudgetUsed) << ",\n";
                out << "                  \"score\": " << format_double(finalBest.second.getScore()) << ",\n";
                out << "                  \"allocation\": ";
                write_int_array(out, finalBest.first.getAllocation());
                out << "\n";
                out << "                }";
                if (stepIdx + 1 < stepTargets.size())
                {
                    out << ",";
                }
                out << "\n";

                ++completedWorkUnits;
                update_progress(progressBar, completedWorkUnits, totalWorkUnits);
            }

            out << "              ],\n";
            out << "              \"finalScore\": " << format_double(finalBest.second.getScore()) << ",\n";
            out << "              \"finalAllocation\": ";
            write_int_array(out, finalBest.first.getAllocation());
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
        out << "    }";
        if (expIndex + 1 < experimentGrid.size())
        {
            out << ",";
        }
        out << "\n";
    }

    out << "  ]\n";
    out << "}\n";

    out.close();

    progressBar.set_option(indicators::option::PostfixText{"Running experiments..."});

    const auto programEnd = std::chrono::steady_clock::now();
    const auto totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(programEnd - programStart);

    std::cout << "[Experiments] Completed " << experimentGrid.size() << " experiment parameter set(s).\n";
    std::cout << "[Experiments] Results written to " << outputPath.string() << "\n";
    std::cout << "[Experiments] Total runtime: " << format_duration(totalDuration) << "\n";

    return EXIT_SUCCESS;
}