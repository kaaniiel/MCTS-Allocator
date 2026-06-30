#ifndef SWITCHPOLICY_HPP
#define SWITCHPOLICY_HPP

#include "../IPolicy.hpp"
#include "../PolicyRegistry.hpp"

class SwitchPolicy : public IPolicy
{
private:
    bool switch_state = false; // Variable to keep track of the current state
public:
    SwitchPolicy(const Config &config) {};

    ~SwitchPolicy() override = default;

    double get_ratio_limit(const Allocation currentAllocation) override
    {
        if (switch_state)
        {
            switch_state = false; // Switch to the other state for the next call
            return 1.0;           // Return 1.0 when in the "on" state
        }
        else
        {
            switch_state = true; // Switch to the other state for the next call
            return 0.0;          // Return 0.0 when in the "off" state
        }
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

inline PolicyRegistrar<SwitchPolicy> regSwitch("SwitchPolicy");
#endif // SWITCHPOLICY_HPP