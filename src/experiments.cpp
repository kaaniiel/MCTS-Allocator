#include <algorithm>
#include <atomic>
#include <chrono>
#include <clocale>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include "config/config.hpp"
#include "indicators/progress_bar.hpp"
#include "mcts/MCTS.hpp"
#include "Solver.hpp"

namespace
{

    struct IterationSnapshot
    {
        int iterationIndex = 0;
        long long elapsedMilliseconds = 0;
        double bestScore = 0.0;
        std::vector<int> bestAllocation;
    };

    struct ChunkSnapshot
    {
        int budget = 0;
        double bestScore = 0.0;
        std::vector<int> bestAllocation;
        std::vector<IterationSnapshot> iterations;
    };

    struct SolverRunResult
    {
        bool succeeded = false;
        long long elapsedMilliseconds = 0;
        double bestScore = 0.0;
        std::vector<int> bestAllocation;
        std::string error;
    };

    indicators::ProgressBar createProgressBar(const std::string &postfixText)
    {
        return indicators::ProgressBar{
            indicators::option::BarWidth{50},
            indicators::option::Start{"["},
            indicators::option::End{"]"},
            indicators::option::Fill{"\u2588"},
            indicators::option::Lead{"\u2588"},
            indicators::option::Remainder{"-"},
            indicators::option::ForegroundColor{indicators::Color::green},
            indicators::option::PostfixText{postfixText},
            indicators::option::ShowPercentage{true},
            indicators::option::ShowElapsedTime{true},
            indicators::option::ShowRemainingTime{true},
            indicators::option::FontStyles{std::vector<indicators::FontStyle>{indicators::FontStyle::bold}}};
    }

    std::string progressSideText(int current,
                                 int total,
                                 const std::string &phase,
                                 int numAgents,
                                 int numObjects,
                                 int seed,
                                 double ratioRandom,
                                 int budget,
                                 int iterationIndex,
                                 int iterationsPerBudget)
    {
        std::ostringstream oss;
        oss << current << "/" << total
            << " | " << phase
            << " | a=" << numAgents
            << " o=" << numObjects
            << " s=" << seed
            << " r=" << std::fixed << std::setprecision(2) << ratioRandom;

        if (budget > 0)
        {
            oss << " | budget=" << budget;
        }

        if (iterationIndex > 0 && iterationsPerBudget > 0)
        {
            oss << " | it=" << iterationIndex << "/" << iterationsPerBudget;
        }

        return oss.str();
    }

    void updateProgress(indicators::ProgressBar &bar,
                        std::atomic<int> &counter,
                        int totalSteps,
                        const std::string &phase,
                        int numAgents,
                        int numObjects,
                        int seed,
                        double ratioRandom,
                        int stepBudget = 0,
                        int iterationIndex = 0,
                        int iterationsPerBudget = 0)
    {
        int current = ++counter;

        bar.set_option(indicators::option::PostfixText{
            progressSideText(current, totalSteps, phase, numAgents, numObjects, seed, ratioRandom, stepBudget, iterationIndex, iterationsPerBudget)});

        const size_t progress = static_cast<size_t>((static_cast<double>(current) / totalSteps) * 100.0);
        bar.set_progress(progress);
    }

    std::string timestampString()
    {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
#if defined(_WIN32) || defined(_WIN64)
        localtime_s(&tm, &t);
#else
        localtime_r(&t, &tm);
#endif
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S");
        return oss.str();
    }

    void writeAllocationJson(std::ostream &out, const std::vector<int> &allocation)
    {
        out << "[";
        for (size_t index = 0; index < allocation.size(); ++index)
        {
            out << allocation[index];
            if (index + 1 < allocation.size())
            {
                out << ", ";
            }
        }
        out << "]";
    }

    std::string ratioToString(double value)
    {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(3) << value;
        std::string text = oss.str();
        std::replace(text.begin(), text.end(), '.', '_');
        return text;
    }

    int maxMctsBudget(int numObjects, int numAgents)
    {
        if (numObjects <= 0 || numAgents <= 0)
        {
            return 1;
        }

        const long double maxRunsLd = std::pow(static_cast<long double>(numObjects), static_cast<long double>(numAgents));
        if (!std::isfinite(static_cast<double>(maxRunsLd)))
        {
            return std::numeric_limits<int>::max();
        }

        const long double capped = std::min(maxRunsLd, static_cast<long double>(std::numeric_limits<int>::max()));
        return std::max(1, static_cast<int>(capped));
    }

    std::vector<int> buildBudgetSchedule(int numObjects, int numAgents)
    {
        std::vector<int> schedule;
        const int maxBudget = maxMctsBudget(numObjects, numAgents);
        int currentBudget = std::max(1, numObjects);

        while (currentBudget > 0 && currentBudget <= maxBudget)
        {
            schedule.push_back(currentBudget);

            if (numObjects <= 1)
            {
                break;
            }

            if (currentBudget > maxBudget / numObjects)
            {
                break;
            }

            currentBudget *= numObjects;
        }

        if (schedule.empty())
        {
            schedule.push_back(1);
        }

        return schedule;
    }

    std::string jsonEscape(const std::string &value)
    {
        std::string escaped;
        escaped.reserve(value.size());
        for (char ch : value)
        {
            switch (ch)
            {
            case '\\':
                escaped += "\\\\";
                break;
            case '"':
                escaped += "\\\"";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            default:
                escaped += ch;
                break;
            }
        }
        return escaped;
    }

    void writeJsonExperimentFile(const std::string &outputPath,
                                 const ExperimentsConfig &config,
                                 int numAgents,
                                 int numObjects,
                                 int seed,
                                 double ratioRandom,
                                 int iterationsPerBudget,
                                 const std::vector<int> &budgetSchedule,
                                 const SolverRunResult &solver,
                                 const Preferences<int> &preferences,
                                 double finalBestScore,
                                 const std::vector<int> &finalBestAllocation,
                                 const std::vector<ChunkSnapshot> &chunks,
                                 int maxBudget)
    {
        std::ofstream file(outputPath);
        if (!file.is_open())
        {
            throw std::runtime_error("Unable to write results file: " + outputPath);
        }

        file << "{\n";
        file << "  \"parameters\": {\n";
        file << "    \"num_agents\": " << numAgents << ",\n";
        file << "    \"num_objects\": " << numObjects << ",\n";
        file << "    \"seed\": " << seed << ",\n";
        file << "    \"ratio_random\": " << ratioRandom << ",\n";
        file << "    \"iterations_per_budget\": " << iterationsPerBudget << ",\n";
        file << "    \"max_budget\": " << maxBudget << ",\n";
        file << "    \"budget_levels\": [";
        for (size_t i = 0; i < budgetSchedule.size(); ++i)
        {
            file << budgetSchedule[i];
            if (i + 1 < budgetSchedule.size())
            {
                file << ", ";
            }
        }
        file << "]\n";
        file << "  },\n";

        file << "  \"preferences\": [\n";
        for (int agent = 0; agent < preferences.getNumAgents(); ++agent)
        {
            file << "    [";
            for (int object = 0; object < preferences.getNumObjects(); ++object)
            {
                file << preferences.getPreference(agent, object);
                if (object + 1 < preferences.getNumObjects())
                {
                    file << ", ";
                }
            }
            file << "]";
            if (agent + 1 < preferences.getNumAgents())
            {
                file << ",";
            }
            file << "\n";
        }
        file << "  ],\n";

        file << "  \"solver\": {\n";
        file << "    \"succeeded\": " << (solver.succeeded ? "true" : "false") << ",\n";
        file << "    \"elapsed_ms\": " << solver.elapsedMilliseconds;
        if (solver.succeeded)
        {
            file << ",\n";
            file << "    \"best_score\": " << solver.bestScore << ",\n";
            file << "    \"best_allocation\": ";
            writeAllocationJson(file, solver.bestAllocation);
            file << "\n";
        }
        else
        {
            file << ",\n";
            file << "    \"error\": \"" << jsonEscape(solver.error) << "\"\n";
        }
        file << "  },\n";

        file << "  \"mcts\": {\n";
        file << "    \"final_best_score\": " << finalBestScore << ",\n";
        file << "    \"final_best_allocation\": ";
        writeAllocationJson(file, finalBestAllocation);
        file << ",\n";
        file << "    \"chunks\": [\n";
        for (size_t index = 0; index < chunks.size(); ++index)
        {
            const ChunkSnapshot &snapshot = chunks[index];
            file << "      {\n";
            file << "        \"budget\": " << snapshot.budget << ",\n";
            file << "        \"best_score\": " << snapshot.bestScore << ",\n";
            file << "        \"best_allocation\": ";
            writeAllocationJson(file, snapshot.bestAllocation);
            file << ",\n";
            file << "        \"iterations\": [\n";
            for (size_t iterationIndex = 0; iterationIndex < snapshot.iterations.size(); ++iterationIndex)
            {
                const IterationSnapshot &iteration = snapshot.iterations[iterationIndex];
                file << "          {\n";
                file << "            \"iteration_index\": " << iteration.iterationIndex << ",\n";
                file << "            \"elapsed_ms\": " << iteration.elapsedMilliseconds << ",\n";
                file << "            \"best_score\": " << iteration.bestScore << ",\n";
                file << "            \"best_allocation\": ";
                writeAllocationJson(file, iteration.bestAllocation);
                file << "\n";
                file << "          }";
                if (iterationIndex + 1 < snapshot.iterations.size())
                {
                    file << ",";
                }
                file << "\n";
            }
            file << "        ]\n";
            file << "      }";
            if (index + 1 < chunks.size())
            {
                file << ",";
            }
            file << "\n";
        }
        file << "    ]\n";
        file << "  }\n";
        file << "}\n";
    }

    class Experiments
    {
    private:
        ExperimentsConfig config;

    public:
        explicit Experiments(ExperimentsConfig config) : config(std::move(config)) {}

        void runExperiments()
        {
#ifdef _WIN32
            SetConsoleOutputCP(CP_UTF8);
            SetConsoleCP(CP_UTF8);
            std::setlocale(LC_ALL, ".UTF-8");
#endif

            if (config.numAgentsMin > config.numAgentsMax || config.numObjectsMin > config.numObjectsMax || config.seedMin > config.seedMax)
            {
                std::cerr << "[Experiments] Invalid range values in config.\n";
                return;
            }

            if (config.ratioRandomStep <= 0.0)
            {
                std::cerr << "[Experiments] Invalid ratio_random_step: " << config.ratioRandomStep << "\n";
                return;
            }

            if (!std::filesystem::exists(config.outputDirectory))
            {
                std::filesystem::create_directories(config.outputDirectory);
            }

            const int agentCount = config.numAgentsMax - config.numAgentsMin + 1;
            const int objectCount = config.numObjectsMax - config.numObjectsMin + 1;
            const int seedCount = config.seedMax - config.seedMin + 1;
            const int ratioCount = std::max(1, static_cast<int>(std::floor((config.ratioRandomMax - config.ratioRandomMin) / config.ratioRandomStep + 0.5)) + 1);

            int totalProgressSteps = 0;
            for (int numAgents = config.numAgentsMin; numAgents <= config.numAgentsMax; ++numAgents)
            {
                for (int numObjects = config.numObjectsMin; numObjects <= config.numObjectsMax; ++numObjects)
                {
                    const std::vector<int> budgetSchedule = buildBudgetSchedule(numObjects, numAgents);
                    totalProgressSteps += seedCount; // one solver step per (agents, objects, seed)
                    totalProgressSteps += seedCount * ratioCount * static_cast<int>(budgetSchedule.size()) * config.iterations;
                }
            }

            auto progressBar = createProgressBar("Running experiments...");
            std::atomic<int> completedSteps{0};

            for (int numAgents = config.numAgentsMin; numAgents <= config.numAgentsMax; ++numAgents)
            {
                for (int numObjects = config.numObjectsMin; numObjects <= config.numObjectsMax; ++numObjects)
                {
                    const std::vector<int> budgetSchedule = buildBudgetSchedule(numObjects, numAgents);
                    const int maxBudget = budgetSchedule.empty() ? std::max(1, numObjects) : budgetSchedule.back();

                    for (int seed = config.seedMin; seed <= config.seedMax; ++seed)
                    {
                        SolverRunResult solverResult;
                        try
                        {
                            Solver<int> solver(numAgents, numObjects, seed);
                            const auto solverStart = std::chrono::steady_clock::now();
                            const std::pair<Allocation, Score> solverBest = solver.solve(config.verbose);
                            const auto solverEnd = std::chrono::steady_clock::now();

                            solverResult.succeeded = true;
                            solverResult.elapsedMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(solverEnd - solverStart).count();
                            solverResult.bestScore = solverBest.second.getScore();
                            solverResult.bestAllocation = solverBest.first.getAllocation();
                        }
                        catch (const std::exception &ex)
                        {
                            solverResult.succeeded = false;
                            solverResult.error = ex.what();
                        }

                        updateProgress(progressBar,
                                       completedSteps,
                                       totalProgressSteps,
                                       "solver",
                                       numAgents,
                                       numObjects,
                                       seed,
                                       0.0,
                                       0,
                                       0,
                                       0);

                        for (int ratioIndex = 0; ratioIndex < ratioCount; ++ratioIndex)
                        {
                            const double ratioRandom = std::min(config.ratioRandomMin + ratioIndex * config.ratioRandomStep, config.ratioRandomMax);

                            std::vector<ChunkSnapshot> chunks;
                            std::vector<int> finalBestAllocation;
                            double finalBestScore = -std::numeric_limits<double>::infinity();
                            Preferences<int> preferencesForJson;
                            bool hasPreferences = false;

                            for (size_t budgetIndex = 0; budgetIndex < budgetSchedule.size(); ++budgetIndex)
                            {
                                const int budget = budgetSchedule[budgetIndex];
                                ChunkSnapshot snapshot;
                                snapshot.budget = budget;

                                for (int iterationIndex = 0; iterationIndex < config.iterations; ++iterationIndex)
                                {
                                    MCTSConfig mctsConfig;
                                    mctsConfig.numAgents = numAgents;
                                    mctsConfig.numObjects = numObjects;
                                    mctsConfig.iterations = budget;
                                    mctsConfig.seed = seed;
                                    mctsConfig.ratioRandom = ratioRandom;
                                    mctsConfig.verbose = config.verbose;
                                    mctsConfig.saveResults = false;
                                    mctsConfig.useSolver = false;

                                    MCTS<int> mcts(mctsConfig);
                                    if (!hasPreferences)
                                    {
                                        preferencesForJson = mcts.getPreferences();
                                        hasPreferences = true;
                                    }

                                    const auto start = std::chrono::steady_clock::now();
                                    mcts.run(budget, false);
                                    const auto end = std::chrono::steady_clock::now();

                                    const auto bestAlloc = mcts.getRoot().getBestAllocation();
                                    IterationSnapshot iterationSnapshot;
                                    iterationSnapshot.iterationIndex = iterationIndex + 1;
                                    iterationSnapshot.elapsedMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
                                    iterationSnapshot.bestScore = bestAlloc.second.getScore();
                                    iterationSnapshot.bestAllocation = bestAlloc.first.getAllocation();
                                    snapshot.iterations.push_back(std::move(iterationSnapshot));

                                    if (bestAlloc.second.getScore() > snapshot.bestScore)
                                    {
                                        snapshot.bestScore = bestAlloc.second.getScore();
                                        snapshot.bestAllocation = bestAlloc.first.getAllocation();
                                    }

                                    if (bestAlloc.second.getScore() > finalBestScore)
                                    {
                                        finalBestScore = bestAlloc.second.getScore();
                                        finalBestAllocation = bestAlloc.first.getAllocation();
                                    }

                                    updateProgress(progressBar,
                                                   completedSteps,
                                                   totalProgressSteps,
                                                   "mcts",
                                                   numAgents,
                                                   numObjects,
                                                   seed,
                                                   ratioRandom,
                                                   budget,
                                                   iterationIndex + 1,
                                                   config.iterations);
                                }

                                chunks.push_back(std::move(snapshot));
                            }

                            const std::string fileName = "experiment_a" + std::to_string(numAgents) +
                                                         "_o" + std::to_string(numObjects) +
                                                         "_s" + std::to_string(seed) +
                                                         "_r" + ratioToString(ratioRandom) +
                                                         "_" + timestampString() + ".json";
                            const std::string outputPath = config.outputDirectory + "/" + fileName;

                            try
                            {
                                writeJsonExperimentFile(outputPath,
                                                        config,
                                                        numAgents,
                                                        numObjects,
                                                        seed,
                                                        ratioRandom,
                                                        config.iterations,
                                                        budgetSchedule,
                                                        solverResult,
                                                        preferencesForJson,
                                                        finalBestScore,
                                                        finalBestAllocation,
                                                        chunks,
                                                        maxBudget);
                                // std::cout << "[Experiments] Saved results to: " << outputPath << "\n";
                            }
                            catch (const std::exception &ex)
                            {
                                std::cerr << "[Experiments] " << ex.what() << "\n";
                            }
                        }
                    }
                }
            }
        }
    };
}

int main()
{
    ExperimentsConfig config;
    try
    {
        config = ExperimentsConfig::load("config.toml");
    }
    catch (const std::exception &ex)
    {
        std::cerr << ex.what() << std::endl;
        ExperimentsConfig::generate_default("config.toml", config);
        std::cerr << "A default configuration file has been generated. Please review and modify 'config.toml' as needed, then re-run the program." << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "\n=== [Experiments] Starting with final configuration ===\n";
    std::cout << " - Num Agents   : " << config.numAgentsMin << ".." << config.numAgentsMax << "\n";
    std::cout << " - Num Objects  : " << config.numObjectsMin << ".." << config.numObjectsMax << "\n";
    std::cout << " - Seed         : " << config.seedMin << ".." << config.seedMax << "\n";
    std::cout << " - Ratio Random : " << config.ratioRandomMin << ".." << config.ratioRandomMax << " (step " << config.ratioRandomStep << ")\n";
    std::cout << " - Iterations   : " << config.iterations << "\n";
    std::cout << " - Verbose      : " << (config.verbose ? "true" : "false") << "\n";
    std::cout << " - Output Dir   : " << config.outputDirectory << "\n";
    std::cout << "====================================================\n\n";

    Experiments experiments(std::move(config));
    experiments.runExperiments();
    return EXIT_SUCCESS;
}