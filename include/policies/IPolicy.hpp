#ifndef IPOLICY_HPP
#define IPOLICY_HPP
#include "mcts/Allocation.hpp"

struct Config; // Forward declaration of the Config struct
class IMCTS;   // Forward declaration of the IMCTS class

class IPolicy
{
public:
    virtual ~IPolicy() = default;

    virtual double get_ratio_limit(const Allocation currentAllocation) = 0;

    virtual void set_config(const Config &config) = 0;

    virtual void set_MCTS_adress(IMCTS *mcts) = 0;
};
#endif // IPOLICY_HPP