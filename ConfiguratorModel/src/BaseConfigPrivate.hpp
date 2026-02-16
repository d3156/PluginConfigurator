#pragma once
#include "BaseConfig.hpp"
namespace d3156
{
    template <typename T>
    inline ConfigField<T>::ConfigField(std::string n, T v, std::string t, Config *parent)
        : name(std::move(n)), value(v), skeletonType(t)
    {
        if (parent) parent->childs.push_back(this);
    }

    template <typename T> inline void ConfigField<T>::addSkeleton(pt::ptree &ptree) const
    {
        ptree.put(name, skeletonType);
    }

    template <typename T> inline void ConfigField<T>::save(pt::ptree &ptree) const { ptree.put(name, value); }

    template <typename T> inline void ConfigField<T>::load(const pt::ptree &ptree)
    {
        value = ptree.get<T>(name, value);
    }

    template <typename T> inline void ConfigArray<T>::addSkeleton(pt::ptree &ptree) const
    {
        ptree.erase(name);
        pt::ptree arr;
        pt::ptree node;
        node.put("", T{});
        arr.push_back(std::make_pair("", node));
        ptree.add_child(name, arr);
    }

    template <typename T> inline void ConfigArray<T>::save(pt::ptree &ptree) const
    {
        ptree.erase(name);
        pt::ptree arr;
        for (const auto &value : items) {
            pt::ptree node;
            node.put("", value);
            arr.push_back(std::make_pair("", node));
        }
        ptree.add_child(name, arr);
    }

    template <typename T> inline void ConfigArray<T>::load(const pt::ptree &ptree)
    {
        items.clear();
        if (auto child = ptree.get_child_optional(name))
            for (const auto &node : *child) items.push_back(node.second.get_value<T>());
    }

    template <typename T> inline ConfigArray<T>::ConfigArray(std::string name_, Config *parent)
    {
        name = name_;
        if (parent) parent->childs.push_back(this);
    }
}