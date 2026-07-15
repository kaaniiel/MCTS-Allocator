#ifndef PREFERENCES_HPP
#define PREFERENCES_HPP

#include <vector>
#include <string>
#include <cstdlib>
#include <iostream>
#include <ctime>
#include <algorithm>
#include <numeric>

/** @brief Class representing the preferences of agents for objects.
 * @tparam T The type used to score preferences (e.g. int, double).
 */
template <typename T>
class Preferences
{
private:
    int numAgents;
    int numObjects;
    std::vector<std::vector<T>> preferences; // A 2D vector where preferences[agent][object] gives the preference score of an agent for an object
    bool verbose;

public:
    /** @brief Default constructor
     */
    Preferences() = default;

    /** @brief Constructor to initialize preferences with all zeros
     * @param numAgents Number of agents
     * @param numObjects Number of objects
     * @param verbose Enable verbose mode
     */
    Preferences(const int numAgents, const int numObjects, const bool verbose = false) : numAgents(numAgents),
                                                                                         numObjects(numObjects),
                                                                                         preferences(numAgents, std::vector<T>(numObjects, static_cast<T>(0))),
                                                                                         verbose(verbose) {}; // Initialize with 0.0 to indicate no preference

    /** @brief Constructor to initialize preferences with all zeros, specifically ignoring the seed parameter during construction but setting totalPerAgents for possible later use (though not stored).
     * @param numAgents Number of agents
     * @param numObjects Number of objects
     * @param seed Seed for random generation (ignored here)
     * @param totalPerAgents Total preference score (ignored here)
     */
    Preferences(const int numAgents, const int numObjects, const int seed, const int totalPerAgents) : numAgents(numAgents),
                                                                                                       numObjects(numObjects),
                                                                                                       preferences(numAgents, std::vector<T>(numObjects, static_cast<T>(0))),
                                                                                                       verbose(false) {}; // Initialize with 0 to indicate no preference

    /** @brief Constructor from an existing 2D vector of preferences and known agent count
     * @param numAgents Number of agents
     * @param prefs The 2D vector of preferences
     */
    Preferences(const int numAgents, const std::vector<std::vector<T>> &prefs) : numAgents(numAgents),
                                                                                 numObjects(prefs.empty() ? 0 : prefs[0].size()),
                                                                                 preferences(prefs),
                                                                                 verbose(false) {};

    /** @brief Constructor from an existing 2D vector of preferences
     * @param prefs The 2D vector of preferences
     */
    Preferences(const std::vector<std::vector<T>> &prefs) : numAgents(prefs.size()),
                                                            numObjects(prefs.empty() ? 0 : prefs[0].size()),
                                                            preferences(prefs),
                                                            verbose(false) {};

    /** @brief Constructor that loads preferences from a file
     * @param filename Path to the file containing preferences
     */
    Preferences(const std::string &filename) : numAgents(0),
                                               numObjects(0),
                                               verbose(false) { loadFromFile(filename); };
    /** @brief Get the preference score of an agent for an object
     * @param agentIndex The index of the agent
     * @param objectIndex The index of the object
     * @return T The preference score of the agent for the object
     */
    T getPreference(int agentIndex, int objectIndex) const { return preferences[agentIndex][objectIndex]; };

    /** @brief Get the preference scores of an agent for all objects
     * @param agentIndex The index of the agent
     * @return std::vector<T> A vector containing the preference scores of the agent for all objects
     */
    std::vector<T> getPreference(int agentIndex) const { return preferences[agentIndex]; };
    /** @brief Set the preference score of an agent for an object
     * @param agentIndex The index of the agent
     * @param objectIndex The index of the object
     * @param score The preference score to set
     * @return void
     */
    void setPreference(int agentIndex, int objectIndex, T score) { preferences[agentIndex][objectIndex] = score; };

    /** @brief Get the full matrix of preferences
     * @return std::vector<std::vector<T>> A 2D vector of all preferences
     */
    std::vector<std::vector<T>> getAllPreferences() const { return preferences; };
    /** @brief Set the verbose mode
     * @param v The verbose mode
     */
    void setVerbose(bool v) { verbose = v; }
    /** @brief Get the verbose mode
     * @return bool The verbose mode
     */
    bool getVerbose() const { return verbose; }
    /** @brief Print the preferences for debugging purposes
     * @return void
     */
    void printPreferences() const
    {
        for (int agent = 0; agent < numAgents; agent++)
        {
            std::cout << "Agent " << agent << " preferences: ";
            for (int object = 0; object < numObjects; ++object)
            {
                std::cout << preferences[agent][object] << " ";
            }
            std::vector<T> agentPrefs = getPreference(agent);
            std::cout << "total : " << std::accumulate(agentPrefs.begin(), agentPrefs.end(), static_cast<T>(0)) << " ";
            std::cout << std::endl;
        }
    }

    /** @brief Generate random preferences for all agents and objects. This can be useful for testing and simulations when you don't have specific preferences to work with. The random values will be between 0 and 1.
     * @param seed The seed for the random number generator to ensure reproducibility
     * @param totalPerAgents The total preference score for each agent. The generated preferences will be scaled to ensure that the sum of preferences for each agent equals this value.
     * @return void
     */
    void generateRandomPreferences(int totalPerAgents, int seed = static_cast<int>(std::time(nullptr)));

    /** @brief Get the number of agents
     * @return int The number of agents
     */
    int getNumAgents() const { return numAgents; };
    /** @brief Get the number of objects
     * @return int The number of objects
     */
    int getNumObjects() const { return numObjects; };

    /** @brief Load preferences from a file. The file should be formatted as follows:
     * The first line contains two integers: the number of agents and the number of objects.
     * The subsequent lines contain the preference scores for each agent and object, with each line corresponding to an agent and containing the preference scores for all objects separated by spaces.
     * @param filename The name of the file to load preferences from
     * @return bool True if the preferences were successfully loaded, false otherwise
     */
    bool loadFromFile(const std::string &filename);

    /** @brief Release internal memory used by preferences.
     */
    void clear()
    {
        preferences.clear();
        preferences.shrink_to_fit();
        numAgents = 0;
        numObjects = 0;
        verbose = false;
    }
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