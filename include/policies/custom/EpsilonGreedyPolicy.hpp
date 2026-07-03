#ifndef EPSILONGREEDYPOLICY_HPP
#define EPSILONGREEDYPOLICY_HPP
#include "../IPolicy.hpp"
#include "../PolicyRegistry.hpp"
#include "../../config/config.hpp"
#include "../../mcts/IMCTS.hpp"
#include <algorithm> // Pour std::clamp

class EpsilonGreedyPolicy : public IPolicy
{
private:
    IMCTS *mcts_ptr = nullptr;
    double epsilon_start;
    double epsilon_end;

    bool is_started = false;
    std::chrono::time_point<std::chrono::steady_clock> start_time;

public:
    // On initialise le point de départ avec la config, et on finit à 0 (100% heuristique à la fin)
    EpsilonGreedyPolicy(const Config &config)
        : epsilon_start(config.ratioRandom), epsilon_end(0.0) {}

    ~EpsilonGreedyPolicy() override = default;

    double get_ratio_limit(const Allocation currentAllocation) override
    {
        if (!mcts_ptr)
            return epsilon_start;

        double progress = 0.0;

        // 1. COMPORTEMENT SI BUDGET EN TEMPS
        if (mcts_ptr->isWorkingWithTimeBudget())
        {
            // On démarre le chrono au premier appel
            if (!is_started)
            {
                start_time = std::chrono::steady_clock::now();
                is_started = true;
            }

            double total_time = mcts_ptr->getTimeBudgetSeconds();
            if (total_time <= 0.0)
                return epsilon_start;

            auto now = std::chrono::steady_clock::now();
            std::chrono::duration<double> elapsed = now - start_time;

            progress = elapsed.count() / total_time;
        }
        // 2. COMPORTEMENT SI BUDGET EN ITÉRATIONS (Classique)
        else
        {
            int current = mcts_ptr->getCurrentIteration();
            int total = mcts_ptr->getTotalIterations();

            if (total <= 0)
                return epsilon_start;

            progress = static_cast<double>(current) / static_cast<double>(total);
        }

        // --- Application de la décroissance ---
        double current_epsilon = epsilon_start - (epsilon_start - epsilon_end) * progress;

        // On bloque entre 0 et 1 (et on empêche epsilon de devenir négatif si le temps dépasse un tout petit peu)
        return std::clamp(current_epsilon, 0.0, 1.0);
    }

    void set_config(const Config &config) override
    {
        epsilon_start = config.ratioRandom;
    }

    void set_MCTS_adress(IMCTS *mcts) override
    {
        mcts_ptr = mcts;
    }
};

// Auto-enregistrement dans le dictionnaire
inline PolicyRegistrar<EpsilonGreedyPolicy> regEpsilonGreedy("EpsilonGreedyPolicy");

#endif // EPSILONGREEDYPOLICY_HPP