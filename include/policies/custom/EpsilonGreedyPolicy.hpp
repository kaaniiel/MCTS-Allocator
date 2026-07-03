#ifndef EPSILONGREEDYPOLICY_HPP
#define EPSILONGREEDYPOLICY_HPP
#include "../IPolicy.hpp"
#include "../PolicyRegistry.hpp"
#include "../../config/config.hpp"
#include "../../mcts/IMCTS.hpp"
#include <algorithm> // Pour std::clamp

/**
 * @brief Epsilon-greedy policy for MCTS that decays the random ratio over time or iterations.
 */
class EpsilonGreedyPolicy : public IPolicy
{
private:
    IMCTS *mcts_ptr = nullptr;
    double epsilon_start;
    double epsilon_end;

    bool is_started = false;
    std::chrono::time_point<std::chrono::steady_clock> start_time;

public:
    /**
     * @brief Constructor initializing the policy with a configuration.
     * Starts with the ratio limit from config and decays to 0 (100% heuristic) at the end.
     * @param config The configuration object containing the initial random ratio.
     */
    EpsilonGreedyPolicy(const Config &config)
        : epsilon_start(config.ratioRandom), epsilon_end(0.0) {}

    ~EpsilonGreedyPolicy() override = default;

    double get_ratio_limit(const Allocation currentAllocation) override
    {
        if (!mcts_ptr)
            return epsilon_start;

        double progress = 0.0;

        // 1. BEHAVIOR WITH TIME BUDGET
        if (mcts_ptr->isWorkingWithTimeBudget())
        {
            // Start the timer on the first call
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
        // 2. BEHAVIOR WITH ITERATION BUDGET (Classic)
        else
        {
            int current = mcts_ptr->getCurrentIteration();
            int total = mcts_ptr->getTotalIterations();

            if (total <= 0)
                return epsilon_start;

            progress = static_cast<double>(current) / static_cast<double>(total);
        }

        // --- Apply the decay ---
        double current_epsilon = epsilon_start - (epsilon_start - epsilon_end) * progress;

        // Clamp between 0 and 1 (prevents epsilon from becoming negative if time slightly exceeds budget)
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

// Auto-register the policy in the PolicyRegistry
inline PolicyRegistrar<EpsilonGreedyPolicy> regEpsilonGreedy("EpsilonGreedyPolicy");

#endif // EPSILONGREEDYPOLICY_HPP