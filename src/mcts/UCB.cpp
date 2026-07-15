#include "mcts/UCB.hpp"

#include <cmath>
#include <limits>
#include <immintrin.h> // SIMD optimizations
double UCB::calculate(const Node &node, const int parentVisits, const double explorationParameter, const bool verbose)
{
    // if the node has not been visited yet, return infinity to ensure it gets selected
    if (node.getVisits() == 0)
    {
        return std::numeric_limits<double>::infinity();
    }

    // Calculate the average reward (exploitation term)
    double averageReward = node.getScore().getScore();
    // Calculate the exploration term
    double explorationTerm = explorationParameter * std::sqrt(static_cast<double>(parentVisits) / node.getVisits());
    // Return the sum of the exploitation and exploration terms
    return averageReward + explorationTerm;
}

Node *UCB::selectBestChild(Node *node, const double explorationParameter, int numAgents, int numObjects, bool truncateTreeSearch, const bool verbose)
{
    if (node == nullptr)
    {
        return nullptr;
    }

    if (node->getChildren().empty())
    {
        return node; // Return the current node if it is not fully expanded
    }

    bool isLeaf = node->isLeafForExpansion(numAgents, numObjects, truncateTreeSearch);
    bool hasMaxChildren = node->getChildren().size() >= static_cast<std::size_t>(node->getMaxChildrenCount(numAgents, numObjects, truncateTreeSearch));
    if (isLeaf || hasMaxChildren)
    {
        return node;
    }

    Node *bestChild = &node->getChildren().front();
    double bestValue = -std::numeric_limits<double>::infinity();

    // Pre-calculate parent visits once
    double parentVisits = static_cast<double>(node->getVisits());

    auto &children = node->getChildren();
    size_t i = 0;
    size_t numChildren = children.size();

#if defined(__AVX2__) && defined(__FMA__)
    // AVX2+FMA setup (__m256d computes four doubles at once)
    const __m256d parentVisits_vec = _mm256_set1_pd(parentVisits);
    const __m256d expParam_vec = _mm256_set1_pd(explorationParameter);

    for (; i + 3 < numChildren; i += 4)
    {
        Node &child0 = children[i];
        Node &child1 = children[i + 1];
        Node &child2 = children[i + 2];
        Node &child3 = children[i + 3];

        if (child0.getVisits() == 0)
            return &child0;
        if (child1.getVisits() == 0)
            return &child1;
        if (child2.getVisits() == 0)
            return &child2;
        if (child3.getVisits() == 0)
            return &child3;

        // _mm256_set_pd(highest..lowest) => lane 0 corresponds to child0 after store
        const __m256d scores = _mm256_set_pd(
            child3.getScore().getScore(),
            child2.getScore().getScore(),
            child1.getScore().getScore(),
            child0.getScore().getScore());

        const __m256d visits = _mm256_set_pd(
            static_cast<double>(child3.getVisits()),
            static_cast<double>(child2.getVisits()),
            static_cast<double>(child1.getVisits()),
            static_cast<double>(child0.getVisits()));

        const __m256d avg_reward = _mm256_div_pd(scores, visits);
        const __m256d inner_div = _mm256_div_pd(parentVisits_vec, visits);
        const __m256d sqrt_val = _mm256_sqrt_pd(inner_div);

        // ucb = avg_reward + expParam * sqrt(parentVisits / visits)
        const __m256d ucb_val = _mm256_fmadd_pd(expParam_vec, sqrt_val, avg_reward);

        alignas(32) double ucb_array[4];
        _mm256_storeu_pd(ucb_array, ucb_val);

        if (ucb_array[0] > bestValue)
        {
            bestValue = ucb_array[0];
            bestChild = &child0;
        }
        if (ucb_array[1] > bestValue)
        {
            bestValue = ucb_array[1];
            bestChild = &child1;
        }
        if (ucb_array[2] > bestValue)
        {
            bestValue = ucb_array[2];
            bestChild = &child2;
        }
        if (ucb_array[3] > bestValue)
        {
            bestValue = ucb_array[3];
            bestChild = &child3;
        }
    }
#elif defined(__AVX__)
    // AVX setup (__m256d computes four doubles at once)
    const __m256d parentVisits_vec = _mm256_set1_pd(parentVisits);
    const __m256d expParam_vec = _mm256_set1_pd(explorationParameter);

    for (; i + 3 < numChildren; i += 4)
    {
        Node &child0 = children[i];
        Node &child1 = children[i + 1];
        Node &child2 = children[i + 2];
        Node &child3 = children[i + 3];

        if (child0.getVisits() == 0)
            return &child0;
        if (child1.getVisits() == 0)
            return &child1;
        if (child2.getVisits() == 0)
            return &child2;
        if (child3.getVisits() == 0)
            return &child3;

        const __m256d scores = _mm256_set_pd(
            child3.getScore().getScore(),
            child2.getScore().getScore(),
            child1.getScore().getScore(),
            child0.getScore().getScore());

        const __m256d visits = _mm256_set_pd(
            static_cast<double>(child3.getVisits()),
            static_cast<double>(child2.getVisits()),
            static_cast<double>(child1.getVisits()),
            static_cast<double>(child0.getVisits()));

        const __m256d avg_reward = _mm256_div_pd(scores, visits);
        const __m256d inner_div = _mm256_div_pd(parentVisits_vec, visits);
        const __m256d sqrt_val = _mm256_sqrt_pd(inner_div);
        const __m256d exp_term = _mm256_mul_pd(expParam_vec, sqrt_val);
        const __m256d ucb_val = _mm256_add_pd(avg_reward, exp_term);

        alignas(32) double ucb_array[4];
        _mm256_storeu_pd(ucb_array, ucb_val);

        if (ucb_array[0] > bestValue)
        {
            bestValue = ucb_array[0];
            bestChild = &child0;
        }
        if (ucb_array[1] > bestValue)
        {
            bestValue = ucb_array[1];
            bestChild = &child1;
        }
        if (ucb_array[2] > bestValue)
        {
            bestValue = ucb_array[2];
            bestChild = &child2;
        }
        if (ucb_array[3] > bestValue)
        {
            bestValue = ucb_array[3];
            bestChild = &child3;
        }
    }
#endif

    // SSE2 fallback for the remaining range (or all children if AVX is unavailable)
    const __m128d parentVisits_vec = _mm_set1_pd(parentVisits);
    const __m128d expParam_vec = _mm_set1_pd(explorationParameter);

    for (; i + 1 < numChildren; i += 2)
    {
        Node &child0 = children[i];
        Node &child1 = children[i + 1];

        if (child0.getVisits() == 0)
            return &child0;
        if (child1.getVisits() == 0)
            return &child1;

        // _mm_set_pd(high, low): high goes to index 1, low goes to index 0
        __m128d scores = _mm_set_pd(child1.getScore().getScore(), child0.getScore().getScore());
        __m128d visits = _mm_set_pd(static_cast<double>(child1.getVisits()), static_cast<double>(child0.getVisits()));

        // averageReward = scores / visits
        __m128d avg_reward = _mm_div_pd(scores, visits);

        // explorationTerm = expParam * sqrt(parentVisits / visits)
        __m128d inner_div = _mm_div_pd(parentVisits_vec, visits);
        __m128d sqrt_val = _mm_sqrt_pd(inner_div);
        __m128d exp_term = _mm_mul_pd(expParam_vec, sqrt_val);

        // ucbValue = averageReward + explorationTerm
        __m128d ucb_val = _mm_add_pd(avg_reward, exp_term);

        double ucb_array[2];
        _mm_storeu_pd(ucb_array, ucb_val);

        if (ucb_array[0] > bestValue)
        {
            bestValue = ucb_array[0];
            bestChild = &child0;
        }
        if (ucb_array[1] > bestValue)
        {
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

        double averageReward = child.getScore().getScore();
        double explorationTerm = explorationParameter * std::sqrt(parentVisits / child.getVisits());
        double ucbValue = averageReward + explorationTerm;

        if (ucbValue > bestValue)
        {
            bestValue = ucbValue;
            bestChild = &child;
        }
    }

    return bestChild;
}