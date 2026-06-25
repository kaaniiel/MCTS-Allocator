#ifndef RANDOMPOLITIC_HPP
#define RANDOMPOLITIC_HPP

#include "IPolitic.hpp"
#include "PoliticRegistry.hpp"
#include <random>     // Pour std::mt19937_64, std::random_device, std::uniform_real_distribution
#include <thread>     // Pour std::this_thread
#include <functional> // Pour std::hash

class RandomUniformPolitic : public IPolitic
{
private:
    // L'ajout de 'inline' (C++17) permet d'initialiser ces variables statiques
    // directement dans le fichier d'en-tête (header) sans erreur de double définition.
    inline static thread_local std::mt19937_64 rng64{
        std::random_device{}() ^ std::hash<std::thread::id>{}(std::this_thread::get_id())};

    inline static thread_local std::uniform_real_distribution<double> dist{0.0, 1.0};

public:
    RandomUniformPolitic() = default;
    ~RandomUniformPolitic() override = default;

    double get_ratio(const int &height) override
    {
        return dist(rng64);
    }
};

inline PoliticRegistrar<RandomUniformPolitic> regRandom("RandomUniformPolitic");

#endif // RANDOMPOLITIC_HPP
