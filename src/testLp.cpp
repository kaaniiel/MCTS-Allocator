#include <iostream>
#include <cmath>
#include <vector>

#include <gurobi_c++.h>

#include "config/config.hpp"
#include "config/CLI11.hpp"
#include "mcts/Preferences.hpp"
#include "metrics/Utility.hpp"
#include "mcts/Allocation.hpp"
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif

int main(int argc, char **argv)
{
    // Force console output to UTF-8
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    std::setlocale(LC_ALL, ".UTF-8");
#endif

    // test the linear programming solver (Gurobi)
    GRBEnv env = GRBEnv(true);
    env.set("LogFile", "gurobi.log");
    env.start();
    GRBModel model = GRBModel(env);
    const int numAgents = 3;
    const int numObjects = 5;
    const int seed = 42;
    const int totalPerAgent = numAgents * numObjects;

    // Use the same data shape/seed logic as MCTS default path.
    Preferences<int> prefs(numAgents, numObjects, false, seed);
    prefs.generateRandomPreferences(totalPerAgent, seed);

    std::cout << "Preferences:" << std::endl;
    prefs.printPreferences();

    // Create Boolean Variables
    GRBVar x[3][5];
    for (int i = 0; i < numAgents; ++i)
    {
        for (int j = 0; j < numObjects; ++j)
        {
            x[i][j] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY, "x_" + std::to_string(i) + "_" + std::to_string(j));
        }
    }

    // Each object must be assigned to exactly one agent (same semantics as Allocation).
    for (int j = 0; j < numObjects; ++j)
    {
        GRBLinExpr objectConstraint = 0;
        for (int i = 0; i < numAgents; ++i)
        {
            objectConstraint += x[i][j];
        }
        model.addConstr(objectConstraint == 1, "Object_" + std::to_string(j) + "_constraint");
    }

    // Match MCTS utility: product over agents of (sum of assigned preferences for that agent).
    // We optimize sum(log(u_i + eps)) which is equivalent to maximizing product(u_i + eps).
    const double eps = 1e-6;
    GRBVar agentUtility[3];
    GRBVar logAgentUtility[3];
    for (int i = 0; i < numAgents; ++i)
    {
        agentUtility[i] = model.addVar(eps, GRB_INFINITY, 0.0, GRB_CONTINUOUS, "u_" + std::to_string(i));
        logAgentUtility[i] = model.addVar(-GRB_INFINITY, GRB_INFINITY, 0.0, GRB_CONTINUOUS, "log_u_" + std::to_string(i));

        GRBLinExpr utilExpr = eps;
        for (int j = 0; j < numObjects; ++j)
        {
            utilExpr += static_cast<double>(prefs.getPreference(i, j)) * x[i][j];
        }
        model.addConstr(agentUtility[i] == utilExpr, "utility_def_" + std::to_string(i));
        model.addGenConstrLog(agentUtility[i], logAgentUtility[i], "log_link_" + std::to_string(i));
    }

    GRBLinExpr objective = 0;
    for (int i = 0; i < numAgents; ++i)
    {
        objective += logAgentUtility[i];
    }
    model.setObjective(objective, GRB_MAXIMIZE);

    // Optimize the model
    model.optimize();

    const int status = model.get(GRB_IntAttr_Status);
    if (status != GRB_OPTIMAL)
    {
        std::cout << "No optimal solution found. Gurobi status = " << status << std::endl;
        return 0;
    }

    // Print the optimal assignment
    std::cout << "Optimal Assignment:" << std::endl;
    std::vector<int> allocVec(numObjects, -1);
    for (int i = 0; i < numAgents; ++i)
    {
        for (int j = 0; j < numObjects; ++j)
        {
            if (x[i][j].get(GRB_DoubleAttr_X) > 0.5) // Check if the variable is set to 1
            {
                const double pref = static_cast<double>(prefs.getPreference(i, j));
                allocVec[j] = i;
                std::cout << "Agent " << i << " assigned to Object " << j << " with preference " << pref << std::endl;
            }
        }
    }

    Allocation alloc(numAgents, allocVec);
    const double mctsUtility = Utility<int>::calculateUtilityMul(prefs, alloc, true);

    // Re-transform from log-space to product-space.
    const double productFromLogs = std::exp(model.get(GRB_DoubleAttr_ObjVal));
    double shiftedProduct = 1.0;
    for (int i = 0; i < numAgents; ++i)
    {
        shiftedProduct *= (agentUtility[i].get(GRB_DoubleAttr_X));
    }

    std::cout << "Objective in log-space (sum logs): " << model.get(GRB_DoubleAttr_ObjVal) << std::endl;
    std::cout << "Product re-transformed with exp(sum logs): " << productFromLogs << std::endl;
    std::cout << "Product from shifted utilities (u_i + eps): " << shiftedProduct << std::endl;
    std::cout << "MCTS utility on same allocation (product of agent sums): " << mctsUtility << std::endl;

    // Show preference matrix
    prefs.printPreferences();
}