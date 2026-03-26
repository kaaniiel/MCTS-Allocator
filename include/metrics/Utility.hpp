#ifndef UTILITY_HPP
#define UTILITY_HPP

#include "Allocation.hpp"

class Utility
{
public:
    /**
     * @brief Calculate the utility value for a given allocation. We multiply the utilities of each agent together to get the overall utility of the allocation. This encourages allocations that are good for all agents rather than just one.
     * @param alloc The allocation for which to calculate the utility
     * @return double The calculated utility value
     */
    static double calculateUtilityMul(const Allocation &alloc);
};
#endif // UTILITY_HPP