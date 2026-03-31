#include "mcts/UCB.hpp"

#include <cmath>
#include <limits>
#include <immintrin.h> // SIMD optimizations
double UCB::calculate(const Node &node, const double explorationParameter, const bool verbose)
{
    // if the node has not been visited yet, return infinity to ensure it gets selected
    if (node.getVisits() == 0)
    {
        return std::numeric_limits<double>::infinity();
    }

    // Calculate the average reward (exploitation term)
    double averageReward = node.getBestAllocation().second.getScore() / node.getVisits();
    // Calculate the exploration term
    double explorationTerm = explorationParameter * std::sqrt(std::log(node.getVisits()) / node.getVisits());
    // Return the sum of the exploitation and exploration terms
    return averageReward + explorationTerm;
}

Node *UCB::selectBestChild(Node *node, const double explorationParameter, const bool verbose)
{
    if (node == nullptr)
    {
        return nullptr;
    }

    if (node->getChildren().empty())
    {
        return node; // Return the current node if it is not fully expanded
    }

    if (node->getChildren().size() < static_cast<size_t>(node->getNumAgents()))
    {
        return node;
    }

    Node *bestChild = &node->getChildren().front();
    double bestValue = -std::numeric_limits<double>::infinity();

    // Pre-calculate expensive logarithm once
    double logVisits = std::log(static_cast<double>(node->getVisits()));
    
    auto &children = node->getChildren();
    size_t i = 0;
    size_t numChildren = children.size();
    
    // SIMD setup (m128d computes two doubles at once)
    __m128d logVisits_vec = _mm_set1_pd(logVisits);
    __m128d expParam_vec = _mm_set1_pd(explorationParameter);
    
    for (; i + 1 < numChildren; i += 2)
    {
        Node &child0 = children[i];
        Node &child1 = children[i+1];
        
        if (child0.getVisits() == 0) return &child0;
        if (child1.getVisits() == 0) return &child1;
        
        // _mm_set_pd(high, low): high goes to index 1, low goes to index 0
        __m128d scores = _mm_set_pd(child1.getBestAllocation().second.getScore(), child0.getBestAllocation().second.getScore());
        __m128d visits = _mm_set_pd(static_cast<double>(child1.getVisits()), static_cast<double>(child0.getVisits()));
        
        // averageReward = scores / visits
        __m128d avg_reward = _mm_div_pd(scores, visits);
        
        // explorationTerm = expParam * sqrt(logVisits / visits)
        __m128d inner_div = _mm_div_pd(logVisits_vec, visits);
        __m128d sqrt_val = _mm_sqrt_pd(inner_div);
        __m128d exp_term = _mm_mul_pd(expParam_vec, sqrt_val);
        
        // ucbValue = averageReward + explorationTerm
        __m128d ucb_val = _mm_add_pd(avg_reward, exp_term);
        
        double ucb_array[2];
        _mm_storeu_pd(ucb_array, ucb_val); // extract 
        
        if (ucb_array[0] > bestValue) {
            bestValue = ucb_array[0];
            bestChild = &child0;
        }
        if (ucb_array[1] > bestValue) {
            bestValue = ucb_array[1];
            bestChild = &child1;
        }
    }

    // Handle remaining child if there is an odd number of children
    for (; i < numChildren; ++i)
    {
        Node &child = children[i];
        if (child.getVisits() == 0)
        {
            return &child;
        }

        double averageReward = child.getBestAllocation().second.getScore() / child.getVisits();
        double explorationTerm = explorationParameter * std::sqrt(logVisits / child.getVisits());
        double ucbValue = averageReward + explorationTerm;

        if (ucbValue > bestValue)
        {
            bestValue = ucbValue;
            bestChild = &child;
        }
    }

    return bestChild;
}