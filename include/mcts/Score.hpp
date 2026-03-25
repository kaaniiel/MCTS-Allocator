#ifndef SCORE_HPP
#define SCORE_HPP
#include <vector>
class Score
{
private:
    // A vector with global score of an allocation.
    // This can be used to store the OWA, the EF score, the utilitarian score, etc. for each allocation.
    std::vector<double> scores;

public:
    Score() = default;
    Score(const std::vector<double> &scores) : scores(scores) {};

    /**
     * @brief Get the score vector
     * @return const std::vector<double>& The score vector
     */
    const std::vector<double> &getScores() const { return scores; };

    /**
     * @brief Set the score vector
     * @param newScores The new score vector to set
     * @return void
     */
    void setScores(const std::vector<double> &newScores) { scores = newScores; };
};

#endif // SCORE_HPP
