#ifndef PREFERENCES_HPP
#define PREFERENCES_HPP

#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <numeric>

template <typename T>
class Preferences
{
private:
    int numAgents;
    int numObjects;
    std::vector<std::vector<T>> preferences; // A 2D vector where preferences[agent][object] gives the preference score of an agent for an object
public:
    Preferences() = default;
    Preferences(const int numAgents, const int numObjects) : numAgents(numAgents),
                                                             numObjects(numObjects),
                                                             preferences(numAgents, std::vector<T>(numObjects, static_cast<T>(0))) {}; // Initialize with 0.0 to indicate no preference
    Preferences(const int numAgents, const int numObjets, const int seed, const int totalPerAgents) : numAgents(numAgents),
                                                                                                      numObjects(numObjets),
                                                                                                      preferences(numAgents, std::vector<T>(numObjets, static_cast<T>(0))) {}; // Initialize with 0 to indicate no preference
    Preferences(const int numAgents, const std::vector<std::vector<T>> &prefs) : numAgents(numAgents),
                                                                                 numObjects(prefs.empty() ? 0 : prefs[0].size()),
                                                                                 preferences(prefs) {};
    Preferences(const std::vector<std::vector<T>> &prefs) : numAgents(prefs.size()),
                                                            numObjects(prefs.empty() ? 0 : prefs[0].size()),
                                                            preferences(prefs) {};

    Preferences(const std::string &filename) : numAgents(0),
                                               numObjects(0) { loadFromFile(filename); };
    /**
     * @brief Get the preference score of an agent for an object
     * @param agentIndex The index of the agent
     * @param objectIndex The index of the object
     * @return T The preference score of the agent for the object
     */
    T getPreference(int agentIndex, int objectIndex) const { return preferences[agentIndex][objectIndex]; };

    /**
     * @brief Get the preference scores of an agent for all objects
     * @param agentIndex The index of the agent
     * @return std::vector<T> A vector containing the preference scores of the agent for all objects
     */
    std::vector<T> getPreference(int agentIndex) const { return preferences[agentIndex]; };
    /**
     * @brief Set the preference score of an agent for an object
     * @param agentIndex The index of the agent
     * @param objectIndex The index of the object
     * @param score The preference score to set
     * @return void
     */
    void setPreference(int agentIndex, int objectIndex, T score) { preferences[agentIndex][objectIndex] = score; };

    /**
     * @brief Generate random preferences for all agents and objects. This can be useful for testing and simulations when you don't have specific preferences to work with. The random values will be between 0 and 1.
     * @param seed The seed for the random number generator to ensure reproducibility
     * @param totalPerAgents The total preference score for each agent. The generated preferences will be scaled to ensure that the sum of preferences for each agent equals this value.
     * @return void
     */
    void generateRandomPreferences(int totalPerAgents, int seed = static_cast<int>(std::time(nullptr)));

    /**
     * @brief Get the number of agents
     * @return int The number of agents
     */
    int getNumAgents() const { return numAgents; };
    /**
     * @brief Get the number of objects
     * @return int The number of objects
     */
    int getNumObjects() const { return numObjects; };

    /**
     * @brief Load preferences from a file. The file should be formatted as follows:
     * The first line contains two integers: the number of agents and the number of objects.
     * The subsequent lines contain the preference scores for each agent and object, with each line corresponding to an agent and containing the preference scores for all objects separated by spaces.
     * @param filename The name of the file to load preferences from
     * @return bool True if the preferences were successfully loaded, false otherwise
     */
    bool loadFromFile(const std::string &filename);
};

template <typename T>
void Preferences<T>::generateRandomPreferences(int totalPerAgents, int seed)
{
    srand(seed);
    for (int i = 0; i < numAgents; ++i)
    {
        T currentSum = 0;
        // Génération
        for (int j = 0; j < numObjects; ++j)
        {
            preferences[i][j] = static_cast<T>(rand()) / RAND_MAX;
            currentSum += preferences[i][j];
        }
        // Mise à l'échelle (règle de trois)
        if (currentSum > 0)
        {
            for (int j = 0; j < numObjects; ++j)
            {
                preferences[i][j] = (preferences[i][j] / currentSum) * static_cast<T>(totalPerAgents);
            }
        }
    }
}

template <>
inline void Preferences<int>::generateRandomPreferences(int totalPerAgents, int seed)
{

    srand(seed);
    for (int i = 0; i < numAgents; ++i)
    {
        // Remise à zéro
        std::fill(preferences[i].begin(), preferences[i].end(), 0);

        // Distribution point par point
        for (int k = 0; k < totalPerAgents; ++k)
        {
            int randomObj = rand() % numObjects;
            preferences[i][randomObj] += 1;
        }
    }
}
#endif // PREFERENCES_HPP