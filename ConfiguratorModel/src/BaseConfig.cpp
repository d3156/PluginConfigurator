#include "BaseConfig.hpp"
namespace d3156
{
    Config::Config(std::string n, Config *parent) : name(std::move(n))
    {
        if (parent) parent->childs.push_back(this);
    }

    void Config::load(const js::object &root)
    {
        const js::object *obj = &root;
        if (!name.empty()) {
            auto *pv = root.if_contains(name);
            if (!pv) return;
            auto *po = pv->if_object();
            if (!po) return;
            obj = po;
        }
        for (IConfig *i : childs) i->load(*obj);
    }

    void Config::save(js::object &root) const
    {
        if (name.empty()) {
            for (IConfig *i : childs) i->save(root);
            return;
        }
        js::object obj;
        for (IConfig *i : childs) i->save(obj);
        root[name] = std::move(obj);
    }

    void Config::addSkeleton(js::object &root) const
    {
        if (name.empty()) {
            for (IConfig *i : childs) i->addSkeleton(root);
            return;
        }
        js::object obj;
        for (IConfig *i : childs) i->addSkeleton(obj);
        root[name] = std::move(obj);
    }
}