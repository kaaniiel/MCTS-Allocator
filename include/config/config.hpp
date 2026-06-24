// include/config/config.hpp
#pragma once
#include <chrono>
#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <functional>
#include <stdexcept>
#include <vector>
#include <type_traits>

#include "toml.hpp"

struct Config
{
    // 1. Default values (Source of truth)
    bool launch = false;
    int numAgents = 3;
    int numObjects = 4;
    int iterations = 100;
    double exploration = 1.414;
    int seed = static_cast<int>(std::chrono::system_clock::now().time_since_epoch().count());
    bool verbose = false;
    double ratioRandom = 1;
    bool saveResults = false;
    bool add_metrics_to_utility = false;
    bool show_metrics = false;
    bool useSolver = false;
    bool terminalJSONOutput = false;
    bool showProgress = false;

    // Default values for experiments
    // GLOBAL
    int numAgentsMin = 3;
    int numAgentsMax = 3;
    std::vector<int> stepAgents = {1};
    int numObjectsMin = 4;
    int numObjectsMax = 4;
    std::vector<int> stepObjects = {1};
    int seedMin = 42;
    int seedMax = 42;
    std::vector<int> stepSeeds = {1};
    bool enableMetrics = false;
    std::string outputDirectory = "results";
    // MCTS
    double budgetMultiplier = 1.0;
    double ratioRandomMin = 1.0;
    double ratioRandomMax = 1.0;
    double ratioRandomStep = 1.0;
    bool useTimeBudget = false;
    double timeBudgetSeconds = 60.0; // 1 minute
    int numberOfTrys = 1;
    double numberOfBudgetStep = 0;
    bool agentHaveMinimumOneObject = false;
    bool uniformizeNegativeValues = false;
    // Solver
    int solverTimeoutSeconds = 60 * 2; // 2 minutes
    bool monitoringCuts = false;

private:
    /**
     * Converts a string so it can be safely written to a TOML file.
     * Special characters are escaped to avoid generating invalid TOML.
     *
     * @param value The input string to convert.
     * @return The escaped string, ready to be written.
     */
    static std::string escape_toml_string(const std::string &value)
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

    /**
     * Writes a one-line TOML comment.
     *
     * @param os The output stream.
     * @param comment The comment text.
     */
    static void write_comment(std::ostream &os, const std::string &comment)
    {
        os << "# " << comment << '\n';
    }

    /**
     * Writes a TOML comment with visual indentation.
     *
     * @param os The output stream.
     * @param indent Visual indentation level, in blocks of 4 spaces.
     * @param comment The comment text.
     */
    static void write_comment(std::ostream &os, std::size_t indent, const std::string &comment)
    {
        os << std::string(indent * 4, ' ') << "# " << comment << '\n';
    }

    /**
     * Writes a TOML table header without indentation.
     *
     * @param os The output stream.
     * @param section The full TOML table name.
     */
    static void write_section(std::ostream &os, const std::string &section)
    {
        os << '\n'
           << '[' << section << "]\n";
    }

    /**
     * Writes a string value in TOML format using the required escaping.
     *
     * @param os The output stream.
     * @param value The value to write.
     */
    static void write_inline_value(std::ostream &os, const std::string &value)
    {
        os << '"' << escape_toml_string(value) << '"';
    }

    /**
     * Convenience overload for C string literals.
     *
     * @param os The output stream.
     * @param value The C-string value to write.
     */
    static void write_inline_value(std::ostream &os, const char *value)
    {
        write_inline_value(os, std::string(value));
    }

    /**
     * Writes a boolean value in TOML format.
     *
     * @param os The output stream.
     * @param value The boolean value to write.
     */
    static void write_inline_value(std::ostream &os, bool value)
    {
        os << (value ? "true" : "false");
    }

    /**
     * Writes a numeric value in TOML format.
     *
     * @param os The output stream.
     * @param value The numeric value to write.
     */
    template <typename T>
    static std::enable_if_t<std::is_arithmetic_v<T> && !std::is_same_v<T, bool>, void> write_inline_value(std::ostream &os, T value)
    {
        os << value;
    }

    /**
     * Writes a scalar key in the form `key = value`.
     *
     * @param os The output stream.
     * @param key The TOML key name.
     * @param value The value to write.
     */
    template <typename T>
    static void write_value(std::ostream &os, const std::string &key, const T &value)
    {
        os << key << " = ";
        write_inline_value(os, value);
        os << '\n';
    }

    /**
     * Writes a scalar key with visual indentation.
     *
     * @param os The output stream.
     * @param indent Visual indentation level, in blocks of 4 spaces.
     * @param key The TOML key name.
     * @param value The value to write.
     */
    template <typename T>
    static void write_value(std::ostream &os, std::size_t indent, const std::string &key, const T &value)
    {
        os << std::string(indent * 4, ' ') << key << " = ";
        write_inline_value(os, value);
        os << '\n';
    }

    /**
     * Writes a TOML array in the form `key = [a, b, c]`.
     *
     * @param os The output stream.
     * @param key The TOML key name.
     * @param values The values to write.
     */
    template <typename T>
    static void write_value(std::ostream &os, const std::string &key, const std::vector<T> &values)
    {
        os << key << " = [";
        for (std::size_t index = 0; index < values.size(); ++index)
        {
            if (index > 0)
            {
                os << ", ";
            }
            write_inline_value(os, values[index]);
        }
        os << "]\n";
    }

    /**
     * Writes a TOML array with visual indentation.
     *
     * @param os The output stream.
     * @param indent Visual indentation level, in blocks of 4 spaces.
     * @param key The TOML key name.
     * @param values The values to write.
     */
    template <typename T>
    static void write_value(std::ostream &os, std::size_t indent, const std::string &key, const std::vector<T> &values)
    {
        os << std::string(indent * 4, ' ') << key << " = [";
        for (std::size_t index = 0; index < values.size(); ++index)
        {
            if (index > 0)
            {
                os << ", ";
            }
            write_inline_value(os, values[index]);
        }
        os << "]\n";
    }

    /**
     * Reads an integer array from a TOML subsection.
     * If the key does not exist at the expected level, the method tries flatter forms before falling back.
     *
     * @param tbl The already parsed TOML table.
     * @param scope The parent table name.
     * @param subscope The nested table name.
     * @param key The key to read.
     * @param fallback The fallback value if the key is missing.
     * @return The read integer vector, or the fallback value.
     */
    static std::vector<int> read_int_vector(const toml::table &tbl, const char *scope, const char *subscope, const char *key, const std::vector<int> &fallback)
    {
        if (auto arr = tbl[scope][subscope][key].as_array())
        {
            std::vector<int> values;
            values.reserve(arr->size());
            for (const auto &elem : *arr)
            {
                values.push_back(static_cast<int>(elem.value_or(static_cast<int64_t>(0))));
            }
            return values;
        }

        if (auto arr = tbl[scope][key].as_array())
        {
            std::vector<int> values;
            values.reserve(arr->size());
            for (const auto &elem : *arr)
            {
                values.push_back(static_cast<int>(elem.value_or(static_cast<int64_t>(0))));
            }
            return values;
        }

        if (auto arr = tbl[key].as_array())
        {
            std::vector<int> values;
            values.reserve(arr->size());
            for (const auto &elem : *arr)
            {
                values.push_back(static_cast<int>(elem.value_or(static_cast<int64_t>(0))));
            }
            return values;
        }

        return fallback;
    }

    /**
     * Reads an integer from a TOML subsection.
     *
     * @param tbl The already parsed TOML table.
     * @param scope The parent table name.
     * @param subscope The nested table name.
     * @param key The key to read.
     * @param fallback The fallback value if the key is missing.
     * @return The read value, or the fallback value.
     */
    static int read_int(const toml::table &tbl, const char *scope, const char *subscope, const char *key, int fallback)
    {
        if (auto value = tbl[scope][subscope][key].value<int64_t>())
        {
            return static_cast<int>(*value);
        }
        if (auto value = tbl[scope][key].value<int64_t>())
        {
            return static_cast<int>(*value);
        }
        if (auto value = tbl[key].value<int64_t>())
        {
            return static_cast<int>(*value);
        }
        return fallback;
    }

    /**
     * Reads an integer from a simple TOML table.
     *
     * @param tbl The already parsed TOML table.
     * @param scope The parent table name.
     * @param key The key to read.
     * @param fallback The fallback value if the key is missing.
     * @return The read value, or the fallback value.
     */
    static int read_int(const toml::table &tbl, const char *scope, const char *key, int fallback)
    {
        if (auto value = tbl[scope][key].value<int64_t>())
        {
            return static_cast<int>(*value);
        }
        if (auto value = tbl[key].value<int64_t>())
        {
            return static_cast<int>(*value);
        }
        return fallback;
    }

    /**
     * Reads a floating-point value from a TOML subsection.
     *
     * @param tbl The already parsed TOML table.
     * @param scope The parent table name.
     * @param subscope The nested table name.
     * @param key The key to read.
     * @param fallback The fallback value if the key is missing.
     * @return The read value, or the fallback value.
     */
    static double read_double(const toml::table &tbl, const char *scope, const char *subscope, const char *key, double fallback)
    {
        if (auto value = tbl[scope][subscope][key].value<double>())
        {
            return *value;
        }
        if (auto value = tbl[scope][key].value<double>())
        {
            return *value;
        }
        if (auto value = tbl[key].value<double>())
        {
            return *value;
        }
        return fallback;
    }

    /**
     * Reads a floating-point value from a simple TOML table.
     *
     * @param tbl The already parsed TOML table.
     * @param scope The parent table name.
     * @param key The key to read.
     * @param fallback The fallback value if the key is missing.
     * @return The read value, or the fallback value.
     */
    static double read_double(const toml::table &tbl, const char *scope, const char *key, double fallback)
    {
        if (auto value = tbl[scope][key].value<double>())
        {
            return *value;
        }
        if (auto value = tbl[key].value<double>())
        {
            return *value;
        }
        return fallback;
    }

    /**
     * Reads a boolean from a TOML subsection.
     *
     * @param tbl The already parsed TOML table.
     * @param scope The parent table name.
     * @param subscope The nested table name.
     * @param key The key to read.
     * @param fallback The fallback value if the key is missing.
     * @return The read value, or the fallback value.
     */
    static bool read_bool(const toml::table &tbl, const char *scope, const char *subscope, const char *key, bool fallback)
    {
        if (auto value = tbl[scope][subscope][key].value<bool>())
        {
            return *value;
        }
        if (auto value = tbl[scope][key].value<bool>())
        {
            return *value;
        }
        if (auto value = tbl[key].value<bool>())
        {
            return *value;
        }
        return fallback;
    }

    /**
     * Reads a boolean from a simple TOML table.
     *
     * @param tbl The already parsed TOML table.
     * @param scope The parent table name.
     * @param key The key to read.
     * @param fallback The fallback value if the key is missing.
     * @return The read value, or the fallback value.
     */
    static bool read_bool(const toml::table &tbl, const char *scope, const char *key, bool fallback)
    {
        if (auto value = tbl[scope][key].value<bool>())
        {
            return *value;
        }
        if (auto value = tbl[key].value<bool>())
        {
            return *value;
        }
        return fallback;
    }

    /**
     * Reads a string from a TOML subsection.
     *
     * @param tbl The already parsed TOML table.
     * @param scope The parent table name.
     * @param subscope The nested table name.
     * @param key The key to read.
     * @param fallback The fallback value if the key is missing.
     * @return The read value, or the fallback value.
     */
    static std::string read_string(const toml::table &tbl, const char *scope, const char *subscope, const char *key, const std::string &fallback)
    {
        if (auto value = tbl[scope][subscope][key].value<std::string>())
        {
            return *value;
        }
        if (auto value = tbl[scope][key].value<std::string>())
        {
            return *value;
        }
        if (auto value = tbl[key].value<std::string>())
        {
            return *value;
        }
        return fallback;
    }

    /**
     * Reads a string from a simple TOML table.
     *
     * @param tbl The already parsed TOML table.
     * @param scope The parent table name.
     * @param key The key to read.
     * @param fallback The fallback value if the key is missing.
     * @return The read value, or the fallback value.
     */
    static std::string read_string(const toml::table &tbl, const char *scope, const char *key, const std::string &fallback)
    {
        if (auto value = tbl[scope][key].value<std::string>())
        {
            return *value;
        }
        if (auto value = tbl[key].value<std::string>())
        {
            return *value;
        }
        return fallback;
    }

    /**
     * Rewrites the configuration file in place using the values already loaded.
     * Missing fields are filled with the current `Config` default values.
     *
     * @param filepath Path to the file to rewrite.
     * @param config Source instance containing the values to write.
     */
    static void rewrite_config_file(const std::string &filepath, const Config &config)
    {
        generate_default(filepath, config);
        std::cout << "[Config] Configuration file rewritten with missing values filled in: " << filepath << "\n";
    }

    /**
     * Registers a value as required.
     * If it is missing, it is added to the error list.
     *
     * @param label Human-readable field name.
     * @param present Indicates whether the field is present.
     * @param reader Action to run when the field exists.
     * @param missing List of missing fields.
     */
    template <typename Reader>
    static void require_value(const std::string &label, bool present, const Reader &reader, std::vector<std::string> &missing)
    {
        if (present)
        {
            reader();
            return;
        }

        missing.push_back(label);
    }

    /**
     * Checks a required value located in a TOML subsection, for example `experiments.global`.
     *
     * @param label Human-readable field name.
     * @param tbl The already parsed TOML table.
     * @param scope The parent table name.
     * @param subscope The nested table name.
     * @param key The key to check.
     * @param missing List of missing fields.
     * @param reader Action to run when the field exists.
     */
    template <typename Reader>
    static void require_nested_value(const std::string &label, const toml::table &tbl, const char *scope, const char *subscope, const char *key, std::vector<std::string> &missing, const Reader &reader)
    {
        const bool present = static_cast<bool>(tbl[scope][subscope][key]);
        require_value(label, present, reader, missing);
    }

    /**
     * Checks a required value located in a simple TOML table, for example `mcts`.
     *
     * @param label Human-readable field name.
     * @param tbl The already parsed TOML table.
     * @param scope The parent table name.
     * @param key The key to check.
     * @param missing List of missing fields.
     * @param reader Action to run when the field exists.
     */
    template <typename Reader>
    static void require_nested_value(const std::string &label, const toml::table &tbl, const char *scope, const char *key, std::vector<std::string> &missing, const Reader &reader)
    {
        const bool present = static_cast<bool>(tbl[scope][key]);
        require_value(label, present, reader, missing);
    }

    /**
     * Checks a required value at the root of the TOML file.
     *
     * @param label Human-readable field name.
     * @param tbl The already parsed TOML table.
     * @param key The key to check.
     * @param missing List of missing fields.
     * @param reader Action to run when the field exists.
     */
    template <typename Reader>
    static void require_root_value(const std::string &label, const toml::table &tbl, const char *key, std::vector<std::string> &missing, const Reader &reader)
    {
        const bool present = static_cast<bool>(tbl[key]);
        require_value(label, present, reader, missing);
    }

public:
    /**
     * Generates a full TOML file from a `Config` instance.
     * This method is used both to create a fresh file and to rewrite a partially valid file.
     *
     * @param filepath Path to the TOML file to write.
     * @param default_config Instance containing the values to serialize.
     */
    static void generate_default(const std::string &filepath, const Config &default_config)
    {
        std::ofstream file(filepath);
        if (file.is_open())
        {
            file << "# Automatically generated configuration file\n";

            write_section(file, "mcts");
            write_value(file, "launch", default_config.launch);
            write_comment(file, "Number of agents in the allocation problem");
            write_value(file, "num_agents", default_config.numAgents);
            write_comment(file, "Number of objects in the allocation problem");
            write_value(file, "num_objects", default_config.numObjects);
            write_comment(file, "Number of iterations for the MCTS algorithm");
            write_value(file, "iterations", default_config.iterations);
            write_comment(file, "Exploration constant (C) for the UCB formula");
            write_value(file, "exploration_constant", default_config.exploration);
            write_comment(file, "Random seed for preference generation");
            write_value(file, "seed", default_config.seed);
            write_comment(file, "Enable verbose output for debugging");
            write_value(file, "verbose", default_config.verbose);
            write_comment(file, "Ratio of random simulations");
            write_value(file, "ratio_random", default_config.ratioRandom);
            write_comment(file, "Save results to a JSON file");
            write_value(file, "save_results", default_config.saveResults);
            write_comment(file, "Add metrics to the utility calculation");
            write_value(file, "add_metrics_to_utility", default_config.add_metrics_to_utility);
            write_comment(file, "Show metrics (EF, EFX, Prop, ...) for the best allocation after MCTS run");
            write_value(file, "show_metrics", default_config.show_metrics);
            write_comment(file, "Use the Gurobi solver to find the optimal allocation instead of MCTS");
            write_value(file, "use_solver", default_config.useSolver);
            write_comment(file, "Output results in JSON format to the terminal");
            write_value(file, "terminal_json_output", default_config.terminalJSONOutput);
            write_comment(file, "Show progress information during the MCTS run");
            write_value(file, "show_progress", default_config.showProgress);

            write_section(file, "experiments");
            write_section(file, "experiments.global");
            write_comment(file, 1, "Values shared by every experiment sweep.");
            write_comment(file, 1, "Set the interval of agents");
            write_value(file, 1, "num_agents_min", default_config.numAgentsMin);
            write_value(file, 1, "num_agents_max", default_config.numAgentsMax);
            write_comment(file, 1, "Set the step of agents (a list of successive increments, e.g., 1, 2, 5), [1] means standard step of 1");
            write_value(file, 1, "step_agents", default_config.stepAgents);
            write_comment(file, 1, "Set the interval of objects");
            write_value(file, 1, "num_objects_min", default_config.numObjectsMin);
            write_value(file, 1, "num_objects_max", default_config.numObjectsMax);
            write_comment(file, 1, "Set the step of objects (a list of successive increments, e.g., 1, 2, 5), [1] means standard step of 1");
            write_value(file, 1, "step_objects", default_config.stepObjects);
            write_comment(file, 1, "Set the interval of seeds");
            write_value(file, 1, "seed_min", default_config.seedMin);
            write_value(file, 1, "seed_max", default_config.seedMax);
            write_comment(file, 1, "Set the step of seeds (a list of successive increments, e.g., 1, 2, 5), [1] means standard step of 1");
            write_value(file, 1, "step_seeds", default_config.stepSeeds);
            write_comment(file, 1, "Write or not metrics (EF, EFX, Prop, ...)");
            write_value(file, 1, "enable_metrics", default_config.enableMetrics);
            write_comment(file, 1, "Verbose output during experiments");
            write_value(file, 1, "verbose", default_config.verbose);
            write_comment(file, 1, "Output directory for experiment results");
            write_value(file, 1, "output_directory", default_config.outputDirectory);

            write_section(file, "experiments.mcts");
            write_comment(file, 1, "Values specific to the MCTS experiment sweep.");
            write_value(file, 1, "ratio_random_min", default_config.ratioRandomMin);
            write_value(file, 1, "ratio_random_max", default_config.ratioRandomMax);
            write_value(file, 1, "ratio_random_step", default_config.ratioRandomStep);
            write_comment(file, 1, "Number of try for each configuration (agents, objects, seed, ratio_random)");
            write_value(file, 1, "number_of_trys", default_config.numberOfTrys);
            write_comment(file, 1, "Slice the budget into how many steps (1 means no slicing, 2 means half budget in the first step, half in the second, etc.)");
            write_value(file, 1, "number_of_budget_step", default_config.numberOfBudgetStep);
            write_comment(file, 1, "Budget multiplier to apply at each try. 1 means no change, 2 means double the budget at each try, etc.");
            write_value(file, 1, "budget_multiplier", default_config.budgetMultiplier);
            write_comment(file, 1, "Whether each agent must have at least one object");
            write_value(file, 1, "agent_have_minimum_one_object", default_config.agentHaveMinimumOneObject);
            write_comment(file, 1, "Whether to uniformize negative values.");
            write_comment(file, 1, "If true, negative values will be transformed to how much agent haven't recieved an object");
            write_value(file, 1, "uniformize_negative_values", default_config.uniformizeNegativeValues);
            write_comment(file, 1, "Get how many cuts are made by MCTS");
            write_value(file, 1, "monitoring_cuts", default_config.monitoringCuts);
            write_comment(file, 1, "Whether to use a time budget instead of a number of iterations");
            write_value(file, 1, "use_time_budget", default_config.useTimeBudget);
            write_comment(file, 1, "Time budget for the MCTS in seconds");
            write_value(file, 1, "time_budget_seconds", default_config.timeBudgetSeconds);
            write_comment(file, 1, "Whether to output results in JSON format to the terminal");
            write_value(file, 1, "terminal_json_output", default_config.terminalJSONOutput);

            write_section(file, "experiments.solver");
            write_comment(file, 1, "Values specific to the solver experiment sweep.");
            write_comment(file, 1, "Time limit for the solver in seconds");
            write_value(file, 1, "solver_timeout_seconds", default_config.solverTimeoutSeconds);
            file << '\n';
            std::cout << "[Config] Default file created: " << filepath << "\n";
        }
        else
        {
            std::cerr << "[Config] Error: unable to create " << filepath << "\n";
        }
    }

    /**
     * Loads a configuration from a TOML file.
     * The method preserves existing values, lists missing keys,
     * rewrites `config.toml` in place with the completed values, then stops execution so the user can review the file.
     *
     * @param filepath Path to the TOML file to load.
     * @return A `Config` structure populated from the file.
     */
    static Config load(const std::string &filepath = "config.toml")
    {
        Config config; // Initialized with default values

        if (!std::filesystem::exists(filepath))
        {
            throw std::runtime_error("[Config] Missing configuration file: " + filepath);
        }

        try
        {
            toml::table tbl = toml::parse_file(filepath);
            std::vector<std::string> missingFields;

            if (!tbl["mcts"])
            {
                missingFields.push_back("mcts");
            }
            if (!tbl["experiments"])
            {
                missingFields.push_back("experiments");
            }

            require_nested_value("mcts.launch", tbl, "mcts", "launch", missingFields, [&]
                                 { config.launch = tbl["mcts"]["launch"].value_or(config.launch); });
            require_nested_value("mcts.num_agents", tbl, "mcts", "num_agents", missingFields, [&]
                                 { config.numAgents = static_cast<int>(tbl["mcts"]["num_agents"].value_or(static_cast<int64_t>(config.numAgents))); });
            require_nested_value("mcts.num_objects", tbl, "mcts", "num_objects", missingFields, [&]
                                 { config.numObjects = static_cast<int>(tbl["mcts"]["num_objects"].value_or(static_cast<int64_t>(config.numObjects))); });
            require_nested_value("mcts.iterations", tbl, "mcts", "iterations", missingFields, [&]
                                 { config.iterations = static_cast<int>(tbl["mcts"]["iterations"].value_or(static_cast<int64_t>(config.iterations))); });
            require_nested_value("mcts.exploration_constant", tbl, "mcts", "exploration_constant", missingFields, [&]
                                 { config.exploration = tbl["mcts"]["exploration_constant"].value_or(config.exploration); });
            require_nested_value("mcts.seed", tbl, "mcts", "seed", missingFields, [&]
                                 { config.seed = static_cast<int>(tbl["mcts"]["seed"].value_or(static_cast<int64_t>(config.seed))); });
            require_nested_value("mcts.verbose", tbl, "mcts", "verbose", missingFields, [&]
                                 { config.verbose = tbl["mcts"]["verbose"].value_or(config.verbose); });
            require_nested_value("mcts.ratio_random", tbl, "mcts", "ratio_random", missingFields, [&]
                                 { config.ratioRandom = tbl["mcts"]["ratio_random"].value_or(config.ratioRandom); });
            require_nested_value("mcts.save_results", tbl, "mcts", "save_results", missingFields, [&]
                                 { config.saveResults = tbl["mcts"]["save_results"].value_or(config.saveResults); });
            require_nested_value("mcts.add_metrics_to_utility", tbl, "mcts", "add_metrics_to_utility", missingFields, [&]
                                 { config.add_metrics_to_utility = tbl["mcts"]["add_metrics_to_utility"].value_or(config.add_metrics_to_utility); });
            require_nested_value("mcts.show_metrics", tbl, "mcts", "show_metrics", missingFields, [&]
                                 { config.show_metrics = tbl["mcts"]["show_metrics"].value_or(config.show_metrics); });
            require_nested_value("mcts.use_solver", tbl, "mcts", "use_solver", missingFields, [&]
                                 { config.useSolver = tbl["mcts"]["use_solver"].value_or(config.useSolver); });
            require_nested_value("mcts.terminal_json_output", tbl, "mcts", "terminal_json_output", missingFields, [&]
                                 { config.terminalJSONOutput = tbl["mcts"]["terminal_json_output"].value_or(config.terminalJSONOutput); });
            // Experiments parameters
            require_nested_value("experiments.global.num_agents_min", tbl, "experiments", "global", "num_agents_min", missingFields, [&]
                                 { config.numAgentsMin = read_int(tbl, "experiments", "global", "num_agents_min", config.numAgentsMin); });
            require_nested_value("experiments.global.num_agents_max", tbl, "experiments", "global", "num_agents_max", missingFields, [&]
                                 { config.numAgentsMax = read_int(tbl, "experiments", "global", "num_agents_max", config.numAgentsMax); });
            require_nested_value("experiments.global.step_agents", tbl, "experiments", "global", "step_agents", missingFields, [&]
                                 { config.stepAgents = read_int_vector(tbl, "experiments", "global", "step_agents", config.stepAgents); });

            require_nested_value("experiments.global.num_objects_min", tbl, "experiments", "global", "num_objects_min", missingFields, [&]
                                 { config.numObjectsMin = read_int(tbl, "experiments", "global", "num_objects_min", config.numObjectsMin); });
            require_nested_value("experiments.global.num_objects_max", tbl, "experiments", "global", "num_objects_max", missingFields, [&]
                                 { config.numObjectsMax = read_int(tbl, "experiments", "global", "num_objects_max", config.numObjectsMax); });
            require_nested_value("experiments.global.step_objects", tbl, "experiments", "global", "step_objects", missingFields, [&]
                                 { config.stepObjects = read_int_vector(tbl, "experiments", "global", "step_objects", config.stepObjects); });

            require_nested_value("experiments.global.seed_min", tbl, "experiments", "global", "seed_min", missingFields, [&]
                                 { config.seedMin = read_int(tbl, "experiments", "global", "seed_min", config.seedMin); });
            require_nested_value("experiments.global.seed_max", tbl, "experiments", "global", "seed_max", missingFields, [&]
                                 { config.seedMax = read_int(tbl, "experiments", "global", "seed_max", config.seedMax); });
            require_nested_value("experiments.global.step_seeds", tbl, "experiments", "global", "step_seeds", missingFields, [&]
                                 { config.stepSeeds = read_int_vector(tbl, "experiments", "global", "step_seeds", config.stepSeeds); });

            require_nested_value("experiments.global.enable_metrics", tbl, "experiments", "global", "enable_metrics", missingFields, [&]
                                 { config.enableMetrics = read_bool(tbl, "experiments", "global", "enable_metrics", config.enableMetrics); });
            require_nested_value("experiments.global.verbose", tbl, "experiments", "global", "verbose", missingFields, [&]
                                 { config.verbose = read_bool(tbl, "experiments", "global", "verbose", config.verbose); });
            require_nested_value("experiments.global.output_directory", tbl, "experiments", "global", "output_directory", missingFields, [&]
                                 { config.outputDirectory = read_string(tbl, "experiments", "global", "output_directory", config.outputDirectory); });

            require_nested_value("experiments.mcts.ratio_random_min", tbl, "experiments", "mcts", "ratio_random_min", missingFields, [&]
                                 { config.ratioRandomMin = read_double(tbl, "experiments", "mcts", "ratio_random_min", config.ratioRandomMin); });
            require_nested_value("experiments.mcts.ratio_random_max", tbl, "experiments", "mcts", "ratio_random_max", missingFields, [&]
                                 { config.ratioRandomMax = read_double(tbl, "experiments", "mcts", "ratio_random_max", config.ratioRandomMax); });
            require_nested_value("experiments.mcts.ratio_random_step", tbl, "experiments", "mcts", "ratio_random_step", missingFields, [&]
                                 { config.ratioRandomStep = read_double(tbl, "experiments", "mcts", "ratio_random_step", config.ratioRandomStep); });
            require_nested_value("experiments.mcts.number_of_trys", tbl, "experiments", "mcts", "number_of_trys", missingFields, [&]
                                 { config.numberOfTrys = read_int(tbl, "experiments", "mcts", "number_of_trys", config.numberOfTrys); });
            require_nested_value("experiments.mcts.number_of_budget_step", tbl, "experiments", "mcts", "number_of_budget_step", missingFields, [&]
                                 { config.numberOfBudgetStep = read_double(tbl, "experiments", "mcts", "number_of_budget_step", config.numberOfBudgetStep); });
            require_nested_value("experiments.mcts.budget_multiplier", tbl, "experiments", "mcts", "budget_multiplier", missingFields, [&]
                                 { config.budgetMultiplier = read_double(tbl, "experiments", "mcts", "budget_multiplier", config.budgetMultiplier); });
            require_nested_value("experiments.mcts.agent_have_minimum_one_object", tbl, "experiments", "mcts", "agent_have_minimum_one_object", missingFields, [&]
                                 { config.agentHaveMinimumOneObject = read_bool(tbl, "experiments", "mcts", "agent_have_minimum_one_object", config.agentHaveMinimumOneObject); });
            require_nested_value("experiments.mcts.uniformize_negative_values", tbl, "experiments", "mcts", "uniformize_negative_values", missingFields, [&]
                                 { config.uniformizeNegativeValues = read_bool(tbl, "experiments", "mcts", "uniformize_negative_values", config.uniformizeNegativeValues); });
            require_nested_value("experiments.mcts.monitoring_cuts", tbl, "experiments", "mcts", "monitoring_cuts", missingFields, [&]
                                 { config.monitoringCuts = read_bool(tbl, "experiments", "mcts", "monitoring_cuts", config.monitoringCuts); });
            require_nested_value("experiments.mcts.use_time_budget", tbl, "experiments", "mcts", "use_time_budget", missingFields, [&]
                                 { config.useTimeBudget = read_bool(tbl, "experiments", "mcts", "use_time_budget", config.useTimeBudget); });
            require_nested_value("experiments.mcts.time_budget_seconds", tbl, "experiments", "mcts", "time_budget_seconds", missingFields, [&]
                                 { config.timeBudgetSeconds = read_double(tbl, "experiments", "mcts", "time_budget_seconds", config.timeBudgetSeconds); });

            if (!missingFields.empty())
            {
                std::cerr << "[Config] Missing required configuration values in " << filepath << "\n";
                for (const auto &field : missingFields)
                {
                    std::cerr << "[Config]   - " << field << "\n";
                }
                rewrite_config_file(filepath, config);
                throw std::runtime_error("[Config] Configuration is incomplete. config.toml has been rewritten with the missing values filled in.");
            }

            std::cout << "[Config] Configuration loaded from " << filepath << "\n";
        }
        catch (const toml::parse_error &err)
        {
            // Handle malformed TOML files (e.g., missing brackets)
            std::cerr << "[Config] Critical syntax error in TOML:\n"
                      << err << "\n";
            std::cerr << "[Config] Falling back to default parameters.\n";
        }

        return config;
    }
};