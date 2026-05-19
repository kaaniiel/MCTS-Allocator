#include <iostream>
#include <vector>
#include <cmath>
#include "metrics/Utility.hpp"
#include "mcts/Preferences.hpp"
#include "mcts/Allocation.hpp"

int main()
{
    Preferences<int> prefs({{4, 3, 9, 6, 3},
                            {4, 4, 5, 5, 7},
                            {8, 4, 2, 5, 6},
                            {5, 6, 6, 5, 3},
                            {3, 5, 8, 5, 4}});

    Allocation alloc(5, {2, 1, 0, 3, 2});
    thread_local std::vector<double> agentUtilities;
    if (agentUtilities.size() < static_cast<size_t>(alloc.getNumAgents()))
    {
        agentUtilities.resize(alloc.getNumAgents(), 0.0);
    }
    else
    {
        std::fill(agentUtilities.begin(), agentUtilities.begin() + alloc.getNumAgents(), 0.0);
    }

    for (int object = 0; object < alloc.getNumObjects(); ++object)
    {
        int agent = alloc.getAllocation()[object];
        if (agent >= 0 && agent < alloc.getNumAgents())
        { // Valid agent assignment
            agentUtilities[agent] += prefs.getPreference(agent, object);
        }
    }

    for (int agent = 0; agent < alloc.getNumAgents(); ++agent)
    {
        std::cout << "Agent " << agent << " utility: " << agentUtilities[agent] << std::endl;
    }
    std::cout << "==============================" << std::endl;
    double utility = Utility<int>::calculateUtilityMul(prefs, alloc, true);
    std::cout << "Utility: " << utility << std::endl;

    return 0;
}