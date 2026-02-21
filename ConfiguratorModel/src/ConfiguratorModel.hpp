#pragma once
#include "BaseConfig.hpp"
#include <PluginCore/IModel>
#include <string>
#include <unordered_map>
#include <vector>

class ConfiguratorModel final : public d3156::PluginCore::IModel
{
    std::vector<std::string> configsPaths_;
    std::unordered_map<std::string, std::string> shemes;

public:
    static std::string name();
    void registerArgs(d3156::Args::Builder &bldr) override;
    int deleteOrder() override { return 0; }
    void init() override;
    void postInit() override;
    void registerConfig(const std::string &name, d3156::Config &conf);

    void setCurrent(const std::string &path, const std::string &data);
    std::string getCurrent(const std::string &path);
    std::string getSheme(const std::string &path);
    std::vector<std::string> &configsPaths();
    std::string configsString();
    static std::string readFileToString(const std::string &file);
    static void pretty_print(std::ostream &os, d3156::js::value const &jv, std::string *indent = nullptr);
};