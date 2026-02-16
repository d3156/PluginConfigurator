#pragma once

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <cstddef>
#include <string>
#include <vector>
namespace d3156
{

    namespace pt = boost::property_tree;

    // "string", "uint", "bool", "int", "enum1|enum2"
    // Интерфейс для любого конфиг-объекта
    struct IConfig {
        virtual void addSkeleton(pt::ptree &ptree) const = 0;
        virtual void load(const pt::ptree &ptree)        = 0;
        virtual void save(pt::ptree &ptree) const        = 0;
        virtual ~IConfig()                               = default;
    };

    // Универсальный объект
    struct Config : public IConfig {

        Config(std::string n, Config *parent = nullptr);
        void load(const pt::ptree &ptree) override;
        void save(pt::ptree &ptree) const override;
        void addSkeleton(pt::ptree &ptree) const override;
        std::vector<IConfig *> childs;
        std::string name;
    };

    // Универсальное поле
    template <typename T> struct ConfigField : public IConfig {
        std::string name;
        T value;
        std::string skeletonType;
        ConfigField(std::string n, T v, std::string t, Config *parent = nullptr);
        void load(const pt::ptree &ptree);
        void save(pt::ptree &ptree) const;
        void addSkeleton(pt::ptree &ptree) const;
        operator T &() { return value; }
    };

    using ConfigString = ConfigField<std::string>;
    using ConfigBool   = ConfigField<bool>;
    using ConfigUint   = ConfigField<size_t>;
    using ConfigEnum   = ConfigField<std::string>;

    // Шаблон для массива объектов конфигурации
    template <typename T> struct ConfigArray : public IConfig {
        std::vector<T> items;
        std::string name;

        ConfigArray(std::string name, Config *parent = nullptr);
        void load(const pt::ptree &ptree) override;
        void save(pt::ptree &ptree) const override;
        void addSkeleton(pt::ptree &ptree) const override;
    };
}

#define CONFIG_STRING(NAME, val) d3156::ConfigField<std::string> NAME = {#NAME, val, "string", this}
#define CONFIG_BOOL(NAME, val)   d3156::ConfigField<bool> NAME = {#NAME, val, "bool", this}
#define CONFIG_UINT(NAME, val)   d3156::ConfigField<size_t> NAME = {#NAME, val, "uint", this}
#define CONFIG_ENUM(NAME, val, vals)   d3156::ConfigField<std::string> NAME = {#NAME, val,  vals, this}

#include "BaseConfigPrivate.hpp"