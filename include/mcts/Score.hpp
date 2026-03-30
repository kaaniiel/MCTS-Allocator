#ifndef SCORE_HPP
#define SCORE_HPP
#include <vector>
class Score
{
private:
    // A vector with global score of an allocation.
    // This can be used to store the OWA, the EF score, the utilitarian score, etc. for each allocation.
    std::vector<double> scores;
    bool verbose;

public:
    Score() : scores(std::vector<double>{0.0}), verbose(false) {};
    Score(const std::vector<double> &scores, const bool verbose = false) : scores(scores), verbose(verbose) {};
    Score(const double score, const bool verbose = false) : scores(std::vector<double>{score}), verbose(verbose) {};
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

    /**
     * @brief Set the verbose mode
     * @param v The verbose mode
     */
    void setVerbose(bool v) { verbose = v; }

    /**
     * @brief Get the verbose mode
     * @return bool The verbose mode
     */
    bool getVerbose() const { return verbose; }
};

#endif // SCORE_HPP
