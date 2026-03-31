#ifndef SCORE_HPP
#define SCORE_HPP

class Score
{
private:
    // Global score of an allocation (OWA, EF score, utilitarian score, etc.)
    double score;
    bool verbose;

public:
    Score() : score(0.0), verbose(false) {};
    Score(const double score, const bool verbose = false) : score(score), verbose(verbose) {};

    /**
     * @brief Get the score
     * @return double The score
     */
    double getScore() const { return score; };

    /**
     * @brief Set the score
     * @param newScore The new score to set
     * @return void
     */
    void setScore(const double newScore) { score = newScore; };

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
