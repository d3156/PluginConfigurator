#include "ConfiguratorModel.hpp"
#include <PluginCore/Logger/Log>
#include <boost/property_tree/json_parser.hpp>
#include <filesystem>
#include <src/Logger/Log.hpp>
#include <string>
#include <fstream>

void ConfiguratorModel::init() {}

void ConfiguratorModel::postInit() {}

std::string ConfiguratorModel::readFileToString(const std::string &file)
{
    std::ifstream ifs(file, std::ios::binary);
    if (!ifs) return {};
    return (std::ostringstream() << ifs.rdbuf()).str();
}

void ConfiguratorModel::pretty_print(std::ostream &os, d3156::js::value const &jv, std::string *indent)
{
    std::string indent_;
    if (!indent) indent = &indent_;
    switch (jv.kind()) {
        case d3156::js::kind::object: {
            os << "{\n";
            indent->append(4, ' ');
            auto const &obj = jv.get_object();
            if (!obj.empty()) {
                auto it = obj.begin();
                for (;;) {
                    os << *indent << d3156::js::serialize(it->key()) << " : ";
                    pretty_print(os, it->value(), indent);
                    if (++it == obj.end()) break;
                    os << ",\n";
                }
            }
            os << "\n";
            indent->resize(indent->size() - 4);
            os << *indent << "}";
            break;
        }
        case d3156::js::kind::array: {
            os << "[\n";
            indent->append(4, ' ');
            auto const &arr = jv.get_array();
            if (!arr.empty()) {
                auto it = arr.begin();
                for (;;) {
                    os << *indent;
                    pretty_print(os, *it, indent);
                    if (++it == arr.end()) break;
                    os << ",\n";
                }
            }
            os << "\n";
            indent->resize(indent->size() - 4);
            os << *indent << "]";
            break;
        }
        case d3156::js::kind::string: os << d3156::js::serialize(jv.get_string()); break;
        case d3156::js::kind::uint64:
        case d3156::js::kind::int64:
        case d3156::js::kind::double_: os << jv; break;
        case d3156::js::kind::bool_: os << (jv.get_bool() ? "true" : "false"); break;
        case d3156::js::kind::null: os << "null"; break;
    }
    if (indent->empty()) os << "\n";
}

void ConfiguratorModel::registerArgs(d3156::Args::Builder &bldr) { bldr.setVersion(FULL_NAME); }

std::string ConfiguratorModel::name() { return FULL_NAME; }

std::string ConfiguratorModel::configsString()
{
    std::string paths = "";
    for (int i = 0; i < configsPaths_.size(); i++) {
        paths += "\"" + configsPaths_[i] + "\"";
        if (i != configsPaths_.size() - 1) paths += ",";
    }
    G_LOG(100, "/configs Answer:" << paths);
    return "[" + paths + "]";
}

std::vector<std::string> &ConfiguratorModel::configsPaths() { return configsPaths_; }

std::string ConfiguratorModel::getCurrent(const std::string &path)
{
    return readFileToString("./configs/" + path + ".json");
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
    G_LOG(1, "Add config to configs " << path);
    const auto fullpath = "./configs/" + path + ".json";
    try {
        if (!std::filesystem::exists(fullpath)) {
            d3156::js::object obj;
            c.save(obj);
            std::ofstream ofs(fullpath, std::ios::binary | std::ios::trunc);
            if (!ofs) throw std::runtime_error("Cannot open config for write: " + fullpath);
            pretty_print(ofs, d3156::js::value(std::move(obj)));
        } else {
            const std::string text = readFileToString(fullpath);
            if (text.empty()) throw std::runtime_error("Empty config file: " + fullpath);
            boost::system::error_code ec;
            d3156::js::value jv = d3156::js::parse(text, ec);
            if (ec) throw std::runtime_error("JSON parse error: " + ec.message());
            auto *obj = jv.if_object();
            if (!obj) throw std::runtime_error("Config root must be object: " + fullpath);
            c.load(*obj);
        }
    } catch (const std::exception &e) {
        R_LOG(1, "Error parse JSON config " << e.what() << " on register file: " << path);
    }
    G_LOG(1, "Success parsed JSON config " << path);
    try {
        d3156::js::object skel;
        c.addSkeleton(skel);
        std::ostringstream oss;
        pretty_print(oss, d3156::js::value(std::move(skel)));
        shemes[path] = oss.str();
        G_LOG(1, "Success added skeleton of JSON config " << path);
    } catch (const std::exception &e) {
        R_LOG(1, "Error serializing JSON " << e.what() << " on register config: " << path);
    }
}
