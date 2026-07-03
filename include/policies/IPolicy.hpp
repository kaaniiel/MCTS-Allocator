#ifndef IPOLICY_HPP
#define IPOLICY_HPP
#include "mcts/Allocation.hpp"

struct Config; // Forward declaration of the Config struct
class IMCTS;   // Forward declaration of the IMCTS class

/**
 * @brief Interface for defining MCTS rollout policies.
 */
class IPolicy
{
public:
    virtual ~IPolicy() = default;

    /**
     * @brief Get the random ratio limit based on the current allocation state.
     * @param currentAllocation The current allocation node state.
     * @return double The ratio of random playouts to heuristic playouts.
     */
    virtual double get_ratio_limit(const Allocation currentAllocation) = 0;

    /**
     * @brief Set the configuration for the policy.
     * @param config The global configuration object.
     */
    virtual void set_config(const Config &config) = 0;

    /**
     * @brief Set the address of the IMCTS instance using this policy.
     * @param mcts Pointer to the MCTS instance.
     */
    virtual void set_MCTS_adress(IMCTS *mcts) = 0;
};
#endif // IPOLICY_HPP