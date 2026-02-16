#include "ConfiguratorModel.hpp"
#include <PluginCore/Logger/Log>
#include <boost/property_tree/json_parser.hpp>
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
        paths += "\"" + configsPaths_[i] +  "\"";
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

void ConfiguratorModel::registerConfig(const std::string &path, d3156::Config &c)
{
    configsPaths_.push_back(path);
    try {
        const auto fullpath = "./configs/" + path + ".json";
        if (!std::filesystem::exists(fullpath)) {
            d3156::pt::ptree ptree;
            c.save(ptree);
            boost::property_tree::write_json(fullpath, ptree);
        } else {
            d3156::pt::ptree ptree;
            boost::property_tree::read_json(fullpath, ptree);
            c.load(ptree);
        }
    } catch (const std::exception &e) {
        R_LOG(1, "Error parse JSON config " << e.what() << " on register file: " << path);
    }
    G_LOG(1, "Success parsed JSON config " << path);
    try {
        std::ostringstream oss;
        d3156::pt::ptree ptree;
        c.addSkeleton(ptree);
        boost::property_tree::write_json(oss, ptree); // false = без pretty print
        shemes[path] = oss.str();
        G_LOG(1, "Success added sceleton of JSON config " << path);
    } catch (const std::exception &e) {
        R_LOG(1, "Error serializing JSON " << e.what() << " on register config: " << path);
    }
}
