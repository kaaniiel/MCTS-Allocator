#ifndef IMCTS_HPP
#define IMCTS_HPP

// Une interface pure qui ne contient AUCUN template
#include "Node.hpp"
class IMCTS
{
public:
    virtual ~IMCTS() = default;

    // --- Noeud racine ---
    virtual Node *getRootNode() = 0;
    // --- Progression de la recherche ---
    virtual int getCurrentIteration() const = 0;
    virtual int getTotalIterations() const = 0;

    // --- Temps (si mode Time Budget activé) ---
    virtual bool isWorkingWithTimeBudget() const = 0;
    virtual double getTimeBudgetSeconds() const = 0;

    // --- Dimensions du problème ---
    virtual int getNumberOfAgents() const = 0;
    virtual int getNumberOfObjects() const = 0;

    // --- Paramètres globaux ---
    virtual double getExplorationParameter() const = 0;

    // --- Monitoring avancé ---
    virtual long long getMonitoringCuts() const = 0;
};

#endif // IMCTS_HPP