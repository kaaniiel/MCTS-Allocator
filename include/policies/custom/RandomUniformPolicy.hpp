#ifndef RANDOMPOLICY_HPP
#define RANDOMPOLICY_HPP

#include "../IPolicy.hpp"
#include "../PolicyRegistry.hpp"
#include <random>     // Pour std::mt19937_64, std::random_device, std::uniform_real_distribution
#include <thread>     // Pour std::this_thread
#include <functional> // Pour std::hash

class RandomUniformPolicy : public IPolicy
{
private:
    // L'ajout de 'inline' (C++17) permet d'initialiser ces variables statiques
    // directement dans le fichier d'en-tête (header) sans erreur de double définition.
    inline static thread_local std::mt19937_64 rng64{
        std::random_device{}() ^ std::hash<std::thread::id>{}(std::this_thread::get_id())};

    inline static thread_local std::uniform_real_distribution<double> dist{0.0, 1.0};

public:
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
