#include <iostream>
#include <numeric>

#include "metrics/Utility.hpp"
#include "mcts/Preferences.hpp"
#include "mcts/Allocation.hpp"
#include "mcts/Node.hpp"

int testPreferences()
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
    std::vector<int> allocVec = {0, 1, 2, 1};                         // Example allocation: object 0 to agent 0, object 1 to agent 1, object 2 to agent 2, object 3 to agent 1
    Allocation alloc(numAgents, allocVec);                            // Create an allocation based on the example vector
    double utility = Utility<int>::calculateUtilityMul(prefs, alloc); // Calculate the utility of the allocation based on the preferences
    std::cout << "Utility of the allocation: " << utility << std::endl;
    return EXIT_SUCCESS;
}

int testExtendNode()
{
    int numAgents = 3;
    int numObjects = 4;
    Node node(numAgents, numObjects); // Create a node with 4 agents and 4 objects
    while (true)
    {
        std::optional<Node> childNode = node.extendNode(node); // Attempt to extend the node to create a new child node
        if (childNode.has_value())
        {
            std::cout << "Extended node at height " << childNode->getHeight() << " with allocation: ";
            const std::vector<int> &allocVec = childNode->getCurrentAllocation().getAllocation();
            for (int object : allocVec)
            {
                std::cout << object << " ";
            }
            std::cout << std::endl;
        }
        else
        {
            std::cout << "No more children can be expanded from this node." << std::endl;
            break; // Exit the loop if no more children can be expanded
        }
    }

    return EXIT_SUCCESS;
}
int main()
{
    return testExtendNode();
}