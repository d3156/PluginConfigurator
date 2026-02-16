#include "ConfiguratorModel.hpp"
#include <PluginCore/Logger/Log>
#include <filesystem>
#include <string>
#include <fstream>

void ConfiguratorModel::init() {}

void ConfiguratorModel::postInit() {}

void ConfiguratorModel::registerArgs(d3156::Args::Builder &bldr) { bldr.setVersion(FULL_NAME); }

std::string ConfiguratorModel::name() { return FULL_NAME; }

std::string ConfiguratorModel::configsString()
{
    std::string paths = "";
    for (int i = 0; i < configsPaths_.size(); i++) {
        paths += configsPaths_[i];
        if (i != configsPaths_.size() - 1) paths += ",";
    }
    return "[" + paths + "]";
}

std::vector<std::string> &ConfiguratorModel::configsPaths() { return configsPaths_; }

std::string ConfiguratorModel::getCurrent(const std::string &path)
{
    if (!std::filesystem::exists("./configs/" + path + ".json")) return "";
    return (std::ostringstream() << std::ifstream("./configs/" + path + ".json").rdbuf()).str();
}

std::string ConfiguratorModel::getSheme(const std::string &path) { return shemes[path]; }

void ConfiguratorModel::setCurrent(const std::string &path, const std::string &data)
{
    std::ofstream of("./configs/" + path + ".json");
    if (of)
        of << data;
    else
        R_LOG(1, "Can not write file ./configs/" + path + ".json");
}
