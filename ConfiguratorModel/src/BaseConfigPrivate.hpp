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
        ptree.clear();
        T obj;
        pt::ptree arr;
        pt::ptree node;
        obj.addSkeleton(node);
        arr.push_back(std::make_pair("", node));
        ptree.put(name, arr);
    }

    template <typename T> inline void ConfigArray<T>::save(pt::ptree &ptree) const
    {
        pt::ptree arr;
        for (const auto &obj : items) {
            pt::ptree node;
            obj.save(node);
            arr.push_back(std::make_pair("", node));
        }
        ptree.put(name, arr);
    }

    template <typename T> inline void ConfigArray<T>::load(const pt::ptree &ptree)
    {
        items.clear();

        for (auto &node : ptree.get_child(name, pt::ptree{})) {
            T obj;
            obj.load(node.second);
            items.push_back(std::move(obj));
        }
    }

    template <typename T> inline ConfigArray<T>::ConfigArray(std::string name_, Config *parent)
    {
        name = name_;
        if (parent) parent->childs.push_back(this);
    }
}