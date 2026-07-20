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
#include <map>
/* #include "../policies/PolicyRegistry.hpp"
#include "../metrics/Utility.hpp" */
#include "../metrics/Metrics.hpp"

#include "toml.hpp"

struct Config
{
    // 1. Default values (Source of truth)
    int numAgents = 3;
    int numObjects = 4;
    int iterations = 100;
    double exploration = 1.414;
    int seed = static_cast<int>(std::chrono::system_clock::now().time_since_epoch().count());
    bool verbose = false;
    double ratioRandom = 1;
    std::string evalFunction = "MNW";
    std::string selectedPolicy = "FixedPolicy";
    bool saveResults = false;
    bool add_metrics_to_utility = false;
    bool show_metrics = false;
    bool useSolver = false;
    bool terminalJSONOutput = false;
    bool showProgress = false;
    bool useTimeBudget = false;
    double timeBudgetSeconds = 60.0 * 2; // 2 minutes

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
    int numberOfTrys = 1;
    double numberOfBudgetStep = 0;
    bool agentHaveMinimumOneObject = false;
    bool uniformizeNegativeValues = false;
    bool experimentUseTimeBudget = false;
    bool adaptBudgetWithSolverTimeout = false;
    int experimentTimeBudgetSeconds = 60 * 2; // 2 minutes
    std::string experimentEvalFunction = "MNW";

    std::map<std::string, bool> experimentPolicys;

    // Solver
    int solverTimeoutSeconds = 60 * 2; // 2 minutes
    bool monitoringCuts = false;

    // Metrics weights
    std::map<std::string, double> metricsWeights = {
        {"isParetoOptimal", 1.0},
        {"isProp", 0.0},
        {"isEF", 0.0},
        {"isEFX", 0.5},
        {"isEF1", 1.0}};

    Config()
    {
        // Auto-register any missing metric from the registry with a default weight of 0.0
        for (const auto &[name, func] : getMetricsRegistry<int>())
        {
            if (metricsWeights.find(name) == metricsWeights.end())
            {
                metricsWeights[name] = 0.0;
            }
        }
    }

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
    static void generate_default(const std::string &filepath, const Config &default_config);

    /**
     * Loads a configuration from a TOML file.
     * The method preserves existing values, lists missing keys,
     * rewrites `config.toml` in place with the completed values, then stops execution so the user can review the file.
     *
     * @param filepath Path to the TOML file to load.
     * @return A `Config` structure populated from the file.
     */
    static Config load(const std::string &filepath = "config.toml");
};