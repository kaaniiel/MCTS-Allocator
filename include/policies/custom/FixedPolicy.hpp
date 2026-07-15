#ifndef FIXEDPOLICY_HPP
#define FIXEDPOLICY_HPP

#include "../IPolicy.hpp"
#include "../PolicyRegistry.hpp"
#include "../../config/config.hpp"

/** @brief Policy that returns a fixed random ratio limit.
 */
class FixedPolicy : public IPolicy
{
private:
    const Config &config;
    const double ratioLimit;

public:
    /** @brief Constructor initializing the fixed policy with a configuration.
     * @param config The configuration object containing the fixed random ratio.
     */
    FixedPolicy(const Config &config) : config(config), ratioLimit(config.ratioRandom) {}
    ~FixedPolicy() override = default;

    double get_ratio_limit(const Allocation currentAllocation) override
    {
        // Implementation for fixed politic ratio limit
        return ratioLimit;
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

inline PolicyRegistrar<FixedPolicy> regFixed("FixedPolicy");

#endif // FIXEDPOLICY_HPP
