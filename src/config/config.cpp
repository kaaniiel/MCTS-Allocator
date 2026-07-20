#include "../include/config/config.hpp" // Ajuste le chemin selon ton dossier
#include "../include/policies/PolicyRegistry.hpp"
#include "../include/metrics/Utility.hpp"

void Config::generate_default(const std::string &filepath, const Config &default_config)
{
    std::ofstream file(filepath);
    if (file.is_open())
    {
        file << "# Automatically generated configuration file\n";

        write_section(file, "mcts");
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
        write_comment(file, "Whether to use a time budget instead of a number of iterations");
        write_value(file, "use_time_budget", default_config.useTimeBudget);
        write_comment(file, "Time budget for the MCTS algorithm in seconds (only used if use_time_budget is true)");
        write_value(file, "time_budget_seconds", default_config.timeBudgetSeconds);
        std::string commentEvalFunction = "Select the evaluation function to use. Available options: ";
        int i = 0;
        // TODO: fix this
        for (const auto &[name, func] : Utility<int>::getUtilityRegistry())
        {
            commentEvalFunction += name;
            if (i < Utility<int>::getUtilityRegistry().size() - 1)
                commentEvalFunction += ", ";
            i++;
        }
        write_comment(file, commentEvalFunction);
        write_value(file, "evaluation_function", default_config.evalFunction);

        std::vector<std::string> availablePolicysComment = PolicyRegistry::getInstance().getAvailablePolicys();
        std::string commentPolicy = "Select the politic to use. Available options: ";
        for (size_t i = 0; i < availablePolicysComment.size(); ++i)
        {
            commentPolicy += availablePolicysComment[i];
            if (i < availablePolicysComment.size() - 1)
                commentPolicy += ", ";
        }
        write_comment(file, commentPolicy); // <-- Le commentaire s'écrira avec la liste de tes classes !
        write_value(file, "selected_politic", default_config.selectedPolicy);

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
        write_comment(file, 1, "Set to true to use a time budget instead of a number of iterations for MCTS.");
        write_value(file, 1, "experimentUseTimeBudget", default_config.experimentUseTimeBudget);
        write_comment(file, 1, "Set to true to adapt the budget with the solver timeout for MCTS.");
        write_value(file, 1, "adaptBudgetWithSolverTimeout", default_config.adaptBudgetWithSolverTimeout);
        write_comment(file, 1, "Set to true to use a time budget instead of a number of iterations for MCTS.");
        write_value(file, 1, "experimentTimeBudgetSeconds", default_config.experimentTimeBudgetSeconds);

        write_comment(file, 1, commentEvalFunction);
        write_value(file, 1, "experimentEvalFunction", default_config.experimentEvalFunction);

        write_section(file, "experiments.mcts.politics");
        write_comment(file, 1, "Set to true to include the politic in the experiment sweep, false to exclude it.");
        std::vector<std::string> availablePolicys = PolicyRegistry::getInstance().getAvailablePolicys();
        for (const auto &polName : availablePolicys)
        {
            // Par défaut, on active tout (true), ou on regarde si elle possède déjà une valeur
            bool isEnabled = true;
            auto it = default_config.experimentPolicys.find(polName);
            if (it != default_config.experimentPolicys.end())
            {
                isEnabled = it->second;
            }
            write_value(file, 1, polName, isEnabled);
        }

        write_section(file, "experiments.solver");
        write_comment(file, 1, "Values specific to the solver experiment sweep.");
        write_comment(file, 1, "Time limit for the solver in seconds. Put a negative value to disable the time limit.");
        write_value(file, 1, "solver_timeout_seconds", default_config.solverTimeoutSeconds);

        write_section(file, "metrics_weights");
        write_comment(file, 1, "Bonus added to the utility if the allocation satisfies the metric, only used when `add_metrics_to_utility` is true.");
        for (const auto &[name, weight] : default_config.metricsWeights)
        {
            write_value(file, 1, name, weight);
        }

        file << '\n';
        std::cout << "[Config] Default file created: " << filepath << "\n";
    }
    else
    {
        std::cerr << "[Config] Error: unable to create " << filepath << "\n";
    }
}

Config Config::load(const std::string &filepath)
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
        if (!tbl["metrics_weights"])
        {
            missingFields.push_back("metrics_weights");
        }

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
        require_nested_value("mcts.selected_politic", tbl, "mcts", "selected_politic", missingFields, [&]
                             { config.selectedPolicy = read_string(tbl, "mcts", "selected_politic", config.selectedPolicy); });
        require_nested_value("mcts.use_time_budget", tbl, "mcts", "use_time_budget", missingFields, [&]
                             { config.useTimeBudget = read_bool(tbl, "mcts", "use_time_budget", config.useTimeBudget); });
        require_nested_value("mcts.time_budget_seconds", tbl, "mcts", "time_budget_seconds", missingFields, [&]
                             { config.timeBudgetSeconds = read_double(tbl, "mcts", "time_budget_seconds", config.timeBudgetSeconds); });

        // Metrics weights
        if (tbl["metrics_weights"])
        {
            if (auto weights_table = tbl["metrics_weights"].as_table())
            {
                for (auto &[k, v] : *weights_table)
                {
                    if (v.is_number())
                    {
                        config.metricsWeights[std::string(k.str())] = v.value_or(0.0);
                    }
                }
            }
        }
        require_nested_value("mcts.evaluation_function", tbl, "mcts", "evaluation_function", missingFields, [&]
                             { config.evalFunction = read_string(tbl, "mcts", "evaluation_function", config.evalFunction); });
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

        require_nested_value("experiments.mcts.experimentEvalFunction", tbl, "experiments", "mcts", "experimentEvalFunction", missingFields, [&]
                             { config.evalFunction = read_string(tbl, "experiments", "mcts", "experimentEvalFunction", config.evalFunction); });
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
        require_nested_value("experiments.mcts.experimentUseTimeBudget", tbl, "experiments", "mcts", "experimentUseTimeBudget", missingFields, [&]
                             { config.experimentUseTimeBudget = read_bool(tbl, "experiments", "mcts", "experimentUseTimeBudget", config.experimentUseTimeBudget); });
        require_nested_value("experiments.mcts.adaptBudgetWithSolverTimeout", tbl, "experiments", "mcts", "adaptBudgetWithSolverTimeout", missingFields, [&]
                             { config.adaptBudgetWithSolverTimeout = read_bool(tbl, "experiments", "mcts", "adaptBudgetWithSolverTimeout", config.adaptBudgetWithSolverTimeout); });
        require_nested_value("experiments.mcts.experimentTimeBudgetSeconds", tbl, "experiments", "mcts", "experimentTimeBudgetSeconds", missingFields, [&]
                             { config.experimentTimeBudgetSeconds = read_double(tbl, "experiments", "mcts", "experimentTimeBudgetSeconds", config.experimentTimeBudgetSeconds); });
        std::vector<std::string> availablePolicys = PolicyRegistry::getInstance().getAvailablePolicys();

        bool hasPolicysSection = static_cast<bool>(tbl["experiments"]["mcts"]["politics"]);
        if (!hasPolicysSection)
        {
            missingFields.push_back("experiments.mcts.politics");
        }
        else
        {
            for (const auto &polName : availablePolicys)
            {
                // On vérifie si la clé de cette politique existe dans le fichier TOML
                if (tbl["experiments"]["mcts"]["politics"][polName])
                {
                    config.experimentPolicys[polName] = tbl["experiments"]["mcts"]["politics"][polName].value_or(true);
                }
                else
                {
                    missingFields.push_back("experiments.mcts.politics." + polName);
                    config.experimentPolicys[polName] = true; // Valeur de repli
                }
            }
        }

        require_nested_value("experiments.solver.solver_timeout_seconds", tbl, "experiments", "solver", "solver_timeout_seconds", missingFields, [&]
                             { config.solverTimeoutSeconds = read_double(tbl, "experiments", "solver", "solver_timeout_seconds", config.solverTimeoutSeconds); });
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