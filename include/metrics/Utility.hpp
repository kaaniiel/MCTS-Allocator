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
        double totalUtility = 1.0; // Start with a utility of 1 for multiplication
        const std::vector<int> &allocation = alloc.getAllocation();
        for (int agent = 0; agent < alloc.getNumAgents(); ++agent)
        {
            double agentUtility = 0.0; // Utility for the current agent
            for (int object = 0; object < alloc.getNumObjects(); ++object)
            {
                if (allocation[object] == agent) // If the object is allocated to the current agent
                {
                    agentUtility += prefs.getPreference(agent, object); // Add the preference score for this object to the agent's utility
                }
            }
            totalUtility *= (agentUtility + 1e-6); // Multiply the agent's utility to the total utility, adding a small value to avoid multiplying by zero
        }
        return std::vector<double>{totalUtility};
    }
};
#endif // UTILITY_HPP