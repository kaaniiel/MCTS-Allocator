#include <iostream>
#include <numeric>

#include "mcts/Preferences.hpp"

int main()
{
    int numAgents = 3;
    int numObjects = 4;
    // int seed = 42;
    int totalPerAgents = 10;
    std::cout << "Generating random preferences for " << numAgents << " agents and " << numObjects << " objects with a total preference score of " << totalPerAgents << " for each agent." << std::endl;
    Preferences<int> prefs = Preferences<int>(numAgents, numObjects); // Create preferences for 3 agents and 4 objects
    // Print the generated preference
    prefs.generateRandomPreferences(totalPerAgents); // Generate random preferences for the agents and objects
    std::cout << "Generated Preferences:" << std::endl;
    for (int agent = 0; agent < prefs.getNumAgents(); agent++)
    {
        std::cout << "Agent " << agent << " preferences: ";
        for (int object = 0; object < prefs.getNumObjects(); ++object)
        {
            std::cout << prefs.getPreference(agent, object) << " ";
        }
        std::vector<int> agentPrefs = prefs.getPreference(agent);
        std::cout << "total : " << std::accumulate(agentPrefs.begin(), agentPrefs.end(), 0) << " ";
        std::cout << std::endl;
    }
    return EXIT_SUCCESS;
}