#ifndef UTILITY_HPP
#define UTILITY_HPP
#include <vector>

#include "../mcts/Preferences.hpp"
#include "../mcts/Allocation.hpp"

template <typename T>
class Utility
{
public:
    /**
     * @brief Calculate the utility value for a given allocation. We multiply the utilities of each agent together to get the overall utility of the allocation. This encourages allocations that are good for all agents rather than just one.
     * @param prefs The preferences for the agents and objects
     * @param alloc The allocation for which to calculate the utility
     * @return double The calculated utility value
     */
    static std::vector<double> calculateUtilityMul(const Preferences<T> &prefs, const Allocation &alloc, const bool verbose = false)
    {
        const std::vector<int> &allocation = alloc.getAllocation();
        int numAgents = alloc.getNumAgents();
        int numObjects = alloc.getNumObjects();

        // Pre-compute agent utilities in a single pass (O(n) instead of O(n²))
        std::vector<double> agentUtilities(numAgents, 0.0);
        for (int object = 0; object < numObjects; ++object)
        {
            int agent = allocation[object];
            if (agent >= 0 && agent < numAgents)
            { // Valid agent assignment
                agentUtilities[agent] += prefs.getPreference(agent, object);
            }
        }

        // Multiply agent utilities to get total utility
        double totalUtility = 1.0;
        for (int agent = 0; agent < numAgents; ++agent)
        {
            totalUtility *= (agentUtilities[agent] + 1e-6);
        }
        return std::vector<double>{totalUtility};
    }
};
#endif // UTILITY_HPP