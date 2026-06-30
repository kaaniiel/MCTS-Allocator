#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include "IPolicy.hpp"

struct Config; // Forward declaration to avoid circular dependency

class PolicyRegistry
{
public:
    // Signature de la fonction capable de créer une politique
    using CreatorFunc = std::function<std::unique_ptr<IPolicy>(const Config &)>;

    // Singleton pour avoir un accès global au registre
    static PolicyRegistry &getInstance()
    {
        static PolicyRegistry instance;
        return instance;
    }

    // Ajoute une politique au dictionnaire
    void registerPolicy(const std::string &name, CreatorFunc func)
    {
        politics_[name] = func;
    }

    // Instancie une politique via son nom
    std::unique_ptr<IPolicy> create(const std::string &name, const Config &config) const
    {
        auto it = politics_.find(name);
        if (it != politics_.end())
        {
            return it->second(config);
        }
        return nullptr; // Ou lever une exception std::invalid_argument
    }

    // Récupère la liste de tous les noms enregistrés
    std::vector<std::string> getAvailablePolicys() const
    {
        std::vector<std::string> names;
        for (const auto &pair : politics_)
        {
            names.push_back(pair.first);
        }
        return names;
    }

private:
    PolicyRegistry() = default;
    std::map<std::string, CreatorFunc> politics_;
};

// --- La classe magique pour l'auto-enregistrement ---
template <typename T>
class PolicyRegistrar
{
public:
    PolicyRegistrar(const std::string &name)
    {
        PolicyRegistry::getInstance().registerPolicy(name, [](const Config &config)
                                                     { return std::make_unique<T>(config); });
    }
};