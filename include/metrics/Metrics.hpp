#ifndef METRICS_HPP
#define METRICS_HPP
#include <vector>

#include "../mcts/Preferences.hpp"
#include "../mcts/Allocation.hpp"

template <typename T>
bool isEF(const Preferences<T> &pref, const std::vector<int> &alloc)
{
    const int numAgents = pref.getNumAgents();
    const int numObjects = alloc.size();

    std::vector<T> bundle_utilities(numAgents);

    for (int i = 0; i < numAgents; ++i)
    {
        const auto &pref_i = pref.getPreference(i); // get preferences of agent i for all objects

        // Compute the utility of each agent's bundle for agent i
        std::fill(bundle_utilities.begin(), bundle_utilities.end(), 0);

        // Calculate the utility of each agent's bundle for agent i
        for (int obj = 0; obj < numObjects; ++obj)
        {
            int owner = alloc[obj];
            if (owner >= 0 && owner < numAgents)
            {
                bundle_utilities[owner] += pref_i[obj];
            }
        }

        // Check if agent i envies any other agent's bundle before him
        T my_utility = bundle_utilities[i];
        for (int j = 0; j < i; ++j)
        {
            if (my_utility < bundle_utilities[j])
                return false;
        }

        // Check if agent i envies any other agent's bundle after him
        for (int j = i + 1; j < numAgents; ++j)
        {
            if (my_utility < bundle_utilities[j])
                return false;
        }
    }

    return true;
}

template <typename T>
bool isEF(const Preferences<T> &pref, const Allocation &alloc)
{
    return isEF(pref, alloc.getAllocation());
}

template <typename T>
bool isEFX(const Preferences<T> &pref, const std::vector<int> &alloc)
{
    const int numAgents = pref.getNumAgents();
    const int numObjects = alloc.size();

    // Valeur infinie pour initialiser la recherche du minimum
    const T MAX_VAL = std::numeric_limits<T>::max();

    std::vector<T> bundle_utilities(numAgents);
    std::vector<T> min_positive_val(numAgents);

    for (int i = 0; i < numAgents; ++i)
    {
        const auto &pref_i = pref.getPreference(i);

        std::fill(bundle_utilities.begin(), bundle_utilities.end(), 0);
        std::fill(min_positive_val.begin(), min_positive_val.end(), MAX_VAL);

        for (int obj = 0; obj < numObjects; ++obj)
        {
            int owner = alloc[obj];
            T val = pref_i[obj];
            if (owner >= 0 && owner < numAgents)
            {
                bundle_utilities[owner] += val;

                if (val > 0 && val < min_positive_val[owner])
                {
                    min_positive_val[owner] = val;
                }
            }
        }

        const T my_utility = bundle_utilities[i];

        for (int j = 0; j < numAgents; ++j)
        {
            if (i == j)
                continue;

            T utility_j = bundle_utilities[j];

            if (min_positive_val[j] != MAX_VAL)
            {
                utility_j -= min_positive_val[j];
            }

            if (my_utility < utility_j)
            {
                return false;
            }
        }
    }

    return true;
}

template <typename T>
bool isEFX(const Preferences<T> &pref, const Allocation &alloc)
{
    return isEFX(pref, alloc.getAllocation());
}

template <typename T>
bool isEF1(const Preferences<T> &pref, const std::vector<int> &alloc)
{
    const int numAgents = pref.getNumAgents();
    const int numObjects = alloc.size();

    thread_local std::vector<T> bundle_utilities;
    thread_local std::vector<T> max_val_in_bundle;

    bundle_utilities.resize(numAgents);
    max_val_in_bundle.resize(numAgents);

    for (int i = 0; i < numAgents; ++i)
    {
        const auto &pref_i = pref.getPreference(i);

        std::fill(bundle_utilities.begin(), bundle_utilities.end(), 0);
        std::fill(max_val_in_bundle.begin(), max_val_in_bundle.end(), 0);

        for (int obj = 0; obj < numObjects; ++obj)
        {
            int owner = alloc[obj];
            T val = pref_i[obj];
            if (owner >= 0 && owner < numAgents)
            {
                bundle_utilities[owner] += val;

                if (val > max_val_in_bundle[owner])
                {
                    max_val_in_bundle[owner] = val;
                }
            }
        }

        const T my_utility = bundle_utilities[i];

        for (int j = 0; j < numAgents; ++j)
        {
            if (i == j)
                continue;

            T utility_j = bundle_utilities[j] - max_val_in_bundle[j];

            if (my_utility < utility_j)
            {
                return false;
            }
        }
    }

    return true;
}

template <typename T>
bool isEF1(const Preferences<T> &pref, const Allocation &alloc)
{
    return isEF1(pref, alloc.getAllocation());
}

template <typename T>
bool isProp(const Preferences<T> &pref, const std::vector<int> &alloc)
{
    const int numAgents = pref.getNumAgents();
    const int numObjects = alloc.size();

    std::vector<T> bundle_utilities(numAgents);
    T total_utility = 0;

    for (int i = 0; i < numAgents; ++i)
    {
        const auto &pref_i = pref.getPreference(i);

        std::fill(bundle_utilities.begin(), bundle_utilities.end(), 0);

        for (int obj = 0; obj < numObjects; ++obj)
        {
            int owner = alloc[obj];
            T val = pref_i[obj];
            if (owner >= 0 && owner < numAgents)
            {
                bundle_utilities[owner] += val;
            }
            total_utility += val;
        }
    }

    for (int i = 0; i < numAgents; ++i)
    {
        if (bundle_utilities[i] < total_utility / numAgents)
        {
            return false;
        }
    }

    return true;
}

template <typename T>
bool isProp(const Preferences<T> &pref, const Allocation &alloc)
{
    return isProp(pref, alloc.getAllocation());
}

template <typename T>
bool isParetoOptimal(const Preferences<T> &pref, const std::vector<int> &alloc)
{
    const int numAgents = pref.getNumAgents();
    const int numObjects = alloc.size();

    GRBEnv env = GRBEnv(true);

    env.set("OutputFlag", "0");
    env.set("LogToConsole", "0");
    env.start();
    GRBModel model = GRBModel(env);
    // Create decision variables for the allocation
    std::vector<std::vector<GRBVar>> x(numAgents, std::vector<GRBVar>(numObjects));
    for (int i = 0; i < numAgents; ++i)
    {
        for (int j = 0; j < numObjects; ++j)
        {
            x[i][j] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY, "x_" + std::to_string(i) + "_" + std::to_string(j));
        }
    }

    // Constraints
    // Each object must be allocated to exactly one agent
    for (int j = 0; j < numObjects; ++j)
    {
        GRBLinExpr objectConstraint = 0;
        for (int i = 0; i < numAgents; ++i)
        {
            objectConstraint += x[i][j];
        }
        model.addConstr(objectConstraint == 1, "Object_" + std::to_string(j) + "_constraint");
    }
    // u_i(sum_j x[i][j] * pref[i][j]) >= u_i(sum_j alloc[i][j] * pref[i][j]) for all i (utility of new allocation must be at least as good as current allocation for all agents)
    std::vector<GRBVar> agentUtility(numAgents);
    std::vector<T> allocUtilities(numAgents);
    for (int i = 0; i < numAgents; ++i)
    {
        agentUtility[i] = model.addVar(-GRB_INFINITY, GRB_INFINITY, 0.0, GRB_CONTINUOUS, "u_" + std::to_string(i));

        GRBLinExpr utilExpr = 0;
        for (int j = 0; j < numObjects; ++j)
        {
            utilExpr += static_cast<double>(pref.getPreference(i, j)) * x[i][j];
            if (alloc[j] == i)
            {
                allocUtilities[i] += pref.getPreference(i, j);
            }
        }
        model.addConstr(agentUtility[i] == utilExpr, "utility_def_" + std::to_string(i));

        GRBLinExpr currentUtil = 0;
        for (int j = 0; j < numObjects; ++j)
        {
            int owner = alloc[j];
            if (owner == i)
            {
                currentUtil += static_cast<double>(pref.getPreference(i, j));
            }
        }
        model.addConstr(agentUtility[i] >= currentUtil, "pareto_optimality_" + std::to_string(i));
    }

    // Objective: maximize the sum of utilities
    GRBLinExpr objective = 0;
    for (int i = 0; i < numAgents; ++i)
    {
        objective += agentUtility[i];
    }
    model.setObjective(objective, GRB_MAXIMIZE);
    model.optimize();

    // if one utility is > from alloc utility, then alloc is not Pareto optimal
    // else if == for all, then alloc is Pareto optimal
    if (model.get(GRB_IntAttr_SolCount) > 0)
    {
        for (int i = 0; i < numAgents; ++i)
        {
            double utilValue = agentUtility[i].get(GRB_DoubleAttr_X);
            if (utilValue > allocUtilities[i] + 1e-6) // Adding a small tolerance
            {
                return false; // Found an allocation that is strictly better for at least one agent
            }
        }
    }
    return true; // Pareto optimal
}

template <typename T>
bool isParetoOptimal(const Preferences<T> &pref, const Allocation &alloc)
{
    return isParetoOptimal(pref, alloc.getAllocation());
}
#endif // METRICS_HPP