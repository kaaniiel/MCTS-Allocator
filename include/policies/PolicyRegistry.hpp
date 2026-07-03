#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include "IPolicy.hpp"

struct Config; // Forward declaration to avoid circular dependency

/**
 * @brief Registry for managing and instantiating available MCTS policies.
 */
class PolicyRegistry
{
public:
    /**
     * @brief Signature of the function capable of creating a policy.
     */
    using CreatorFunc = std::function<std::unique_ptr<IPolicy>(const Config &)>;

    /**
     * @brief Singleton access to the global policy registry.
     * @return PolicyRegistry& The singleton instance.
     */
    static PolicyRegistry &getInstance()
    {
        static PolicyRegistry instance;
        return instance;
    }

    /**
     * @brief Register a new policy with its creation function.
     * @param name The name of the policy.
     * @param func The creation function.
     */
    void registerPolicy(const std::string &name, CreatorFunc func)
    {
        politics_[name] = func;
    }

    /**
     * @brief Instantiate a policy by its registered name.
     * @param name The name of the policy to instantiate.
     * @param config The configuration to pass to the policy.
     * @return std::unique_ptr<IPolicy> A pointer to the created policy, or nullptr if not found.
     */
    std::unique_ptr<IPolicy> create(const std::string &name, const Config &config) const
    {
        auto it = politics_.find(name);
        if (it != politics_.end())
        {
            return it->second(config);
        }
        return nullptr; // Ou lever une exception std::invalid_argument
    }

    /**
     * @brief Retrieve the list of all registered policy names.
     * @return std::vector<std::string> The list of policy names.
     */
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

/**
     * @brief Helper class for automatic self-registration of policies.
     * @tparam T The policy class type to register.
     */
template <typename T>
class PolicyRegistrar
{
public:
    /**
     * @brief Constructor that registers the policy with the given name.
     * @param name The name under which to register the policy.
     */
    PolicyRegistrar(const std::string &name)
    {
        PolicyRegistry::getInstance().registerPolicy(name, [](const Config &config)
                                                     { return std::make_unique<T>(config); });
    }
};