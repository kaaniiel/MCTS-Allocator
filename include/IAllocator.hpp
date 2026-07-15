#ifndef IALLOCATOR_HPP
#define IALLOCATOR_HPP

#include <string>
#include <utility>
#include "mcts/Allocation.hpp"
#include "mcts/Score.hpp"
#include "mcts/Preferences.hpp"
#include "config/config.hpp"

template <typename T>
class IAllocator
{
public:
    // 1. Un destructeur virtuel est OBLIGATOIRE pour une interface en C++
    // pour éviter les fuites de mémoire lors de la destruction polymorphique.
    virtual ~IAllocator() = default;

    /** @brief Solve the allocation problem and return the optimal allocation and score
     * @param verbose Whether to print detailed information during the solving process
     * @return std::pair<Allocation, Score> The optimal allocation and its score
     */
    virtual std::pair<Allocation, Score> solve(bool verbose = false) = 0;

    /** @brief Get the preferences used by the allocator
     * @return const Preferences<T> & The preferences used by the allocator
     */
    virtual const Preferences<T> &getPreferences() const = 0;

    /** @brief Clear stored solver results and release memory held by preferences/allocation.
     * @return void
     */
    virtual void clear() = 0; // Clear stored solver results and release memory

    /** @brief Load configuration settings from a Config object
     * @param config The Config object containing the settings to load
     *
     * @return void
     */
    virtual void load_config(const Config &config) = 0; // Load configuration from a Config object

    /** @brief Save the results of the allocation to a JSON file
     * @param filename The name of the file to save the results to
     * @param add_metrics Whether to include metrics in the saved results
     * @return void
     */
    virtual void save_results_json(const std::string &filename, bool add_metrics = false) = 0;

    /** @brief Convert the results of the allocation to a JSON string
     * @param add_metrics Whether to include metrics in the JSON string
     * @return std::string The JSON string containing the allocation results
     */
    virtual std::string to_json(bool add_metrics = false) = 0;
};

#endif // IALLOCATOR_HPP