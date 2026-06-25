#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include "IPolitic.hpp"

class PoliticRegistry
{
public:
    // Signature de la fonction capable de créer une politique
    using CreatorFunc = std::function<std::unique_ptr<IPolitic>()>;

    // Singleton pour avoir un accès global au registre
    static PoliticRegistry &getInstance()
    {
        static PoliticRegistry instance;
        return instance;
    }

    // Ajoute une politique au dictionnaire
    void registerPolitic(const std::string &name, CreatorFunc func)
    {
        politics_[name] = func;
    }

    // Instancie une politique via son nom
    std::unique_ptr<IPolitic> create(const std::string &name) const
    {
        auto it = politics_.find(name);
        if (it != politics_.end())
        {
            return it->second();
        }
        return nullptr; // Ou lever une exception std::invalid_argument
    }

    // Récupère la liste de tous les noms enregistrés
    std::vector<std::string> getAvailablePolitics() const
    {
        std::vector<std::string> names;
        for (const auto &pair : politics_)
        {
            names.push_back(pair.first);
        }
        return names;
    }

private:
    PoliticRegistry() = default;
    std::map<std::string, CreatorFunc> politics_;
};

// --- La classe magique pour l'auto-enregistrement ---
template <typename T>
class PoliticRegistrar
{
public:
    PoliticRegistrar(const std::string &name)
    {
        PoliticRegistry::getInstance().registerPolitic(name, []()
                                                       { return std::make_unique<T>(); });
    }
};