#ifndef RANDOMPOLICY_HPP
#define RANDOMPOLICY_HPP

#include "../IPolicy.hpp"
#include "../PolicyRegistry.hpp"
#include <random>     // Pour std::mt19937_64, std::random_device, std::uniform_real_distribution
#include <thread>     // Pour std::this_thread
#include <functional> // Pour std::hash

/** @brief Policy that returns a completely random uniform ratio limit between 0.0 and 1.0.
 */
class RandomUniformPolicy : public IPolicy
{
private:
    // Adding 'inline' (C++17) allows initializing these static variables
    // directly in the header file without double definition errors.
    inline static thread_local std::mt19937_64 rng64{
        std::random_device{}() ^ std::hash<std::thread::id>{}(std::this_thread::get_id())};

    inline static thread_local std::uniform_real_distribution<double> dist{0.0, 1.0};

public:
    /** @brief Constructor initializing the random policy.
     * @param config The configuration object (unused for this policy).
     */
    RandomUniformPolicy(const Config &config) {};

    ~RandomUniformPolicy() override = default;

    double get_ratio_limit(const Allocation currentAllocation) override
    {
        return dist(rng64);
    }

    void set_config(const Config &config) override
    {
        // No configuration needed for this policy
    }

    void set_MCTS_adress(IMCTS *mcts) override
    {
        // No MCTS address needed for this policy
    }
};

inline PolicyRegistrar<RandomUniformPolicy> regRandom("RandomUniformPolicy");

#endif // RANDOMPOLICY_HPP
