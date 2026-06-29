#ifndef FIXEDPOLITIC_HPP
#define FIXEDPOLITIC_HPP

#include "IPolitic.hpp"
#include "PoliticRegistry.hpp"
#include "../config/Config.hpp"

class FixedPolitic : public IPolitic
{
private:
    const Config &config;
    const double ratioLimit;

public:
    FixedPolitic(const Config &config) : config(config), ratioLimit(config.ratioRandom) {}
    ~FixedPolitic() override = default;

    double get_ratio_limit(const Allocation currentAllocation) override
    {
        // Implementation for fixed politic ratio limit
        return ratioLimit;
    }
};

inline PoliticRegistrar<FixedPolitic> regFixed("FixedPolitic");

#endif // FIXEDPOLITIC_HPP
