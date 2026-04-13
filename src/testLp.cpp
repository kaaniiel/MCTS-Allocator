#include <iostream>
#include <windows.h>
#include <gurobi_c++.h>

#include "config/config.hpp"
#include "config/CLI11.hpp" // Assure-toi que le chemin d'inclusion est correct
#include "mcts/Preferences.hpp"


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
    // Create Preferences object and generate random preferences
    Preferences<int> prefs(3,5,false,10);
    prefs.generateRandomPreferences(10,42);

    // Create Boolean Variables
    GRBVar x[3][5];
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 5; ++j)
        {
            x[i][j] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY, "x_" + std::to_string(i) + "_" + std::to_string(j));
        }
    }

    // Set objective: maximize multiplicative utility based on preferences (with sum of logarithms to avoid numerical issues)
    GRBLinExpr objective = 0;
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 5; ++j)
        {
            objective += prefs.getPreference(i, j) * x[i][j];
        }
    }
    model.setObjective(objective, GRB_MAXIMIZE);
    // Add constraints: each agent can be allocated at most 1 object
    for (int i = 0; i < 3; ++i)
    {
        GRBLinExpr agentConstraint = 0;
        for (int j = 0; j < 5; ++j)
        {
            agentConstraint += x[i][j];
        }
        model.addConstr(agentConstraint <= 1, "agent_" + std::to_string(i) + "_constraint");
    }
    // Add constraints: each object can be allocated to at most 1 agent
    for (int j = 0; j < 5; ++j)
    {
        GRBLinExpr objectConstraint = 0;
        for (int i = 0; i < 3; ++i)
        {
            objectConstraint += x[i][j];
        }
        model.addConstr(objectConstraint <= 1, "object_" + std::to_string(j) + "_constraint");
    }
    // Optimize model
    model.optimize();

    // Print results
    std::cout << "Optimal objective value: " << model.get(GRB_DoubleAttr_ObjVal) << std::endl;
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 5; ++j)
        {
            if (x[i][j].get(GRB_DoubleAttr_X) > 0.5)
            {
                std::cout << "Agent " << i << " allocated to Object " << j << " with preference " << prefs.getPreference(i, j) << std::endl;
            }
        }
    }




}