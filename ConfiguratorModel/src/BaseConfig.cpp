#include "BaseConfig.hpp"
namespace d3156
{
    Config::Config(std::string n, Config *parent) : name(std::move(n))
    {
        if (parent) parent->childs.push_back(this);
    }

    void Config::load(const pt::ptree &ptree)
    {
        auto &obj = name.empty() ? ptree : ptree.get_child(name, pt::ptree{});
        for (IConfig *i : childs) i->load(obj);
    }

    void Config::save(pt::ptree &ptree) const
    {
        if (name.empty()) {
            for (IConfig *i : childs) i->save(ptree);
            return;
        }
        pt::ptree obj;
        for (IConfig *i : childs) i->save(obj);
        ptree.add_child(name, obj);
    }

    void Config::addSkeleton(pt::ptree &ptree) const
    {
        if (name.empty()) {
            for (IConfig *i : childs) i->addSkeleton(ptree);
            return;
        }
        pt::ptree obj{};
        for (IConfig *i : childs) i->addSkeleton(obj);
        ptree.add_child(name, obj);
    }
}