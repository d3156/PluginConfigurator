#pragma once
#include "BaseConfig.hpp"
namespace d3156
{
    template <typename T> inline ConfigField<T>::operator T &() { return value; }

    template <typename T> inline void ConfigField<T>::load(const js::object &obj)
    {
        auto *pv = obj.if_contains(name);
        if (!pv) return;

        if constexpr (std::is_same_v<T, std::string>) {
            if (auto *ps = pv->if_string()) value = ps->c_str();
        } else if constexpr (std::is_same_v<T, bool>) {
            if (pv->is_bool()) value = pv->as_bool();
        } else if constexpr (std::is_integral_v<T> && std::is_unsigned_v<T>) {
            if (pv->is_uint64())
                value = static_cast<T>(pv->as_uint64());
            else if (pv->is_int64() && pv->as_int64() >= 0)
                value = static_cast<T>(pv->as_int64());
        } else if constexpr (std::is_integral_v<T> && std::is_signed_v<T>) {
            if (pv->is_int64())
                value = static_cast<T>(pv->as_int64());
            else if (pv->is_uint64())
                value = static_cast<T>(pv->as_uint64());
        }
    }

    template <typename T> inline void ConfigField<T>::save(js::object &obj) const
    {
        if constexpr (std::is_same_v<T, std::string>)
            obj[name] = value;
        else if constexpr (std::is_same_v<T, bool>)
            obj[name] = value;
        else if constexpr (std::is_integral_v<T> && std::is_unsigned_v<T>)
            obj[name] = static_cast<std::uint64_t>(value);
        else if constexpr (std::is_integral_v<T> && std::is_signed_v<T>)
            obj[name] = static_cast<std::int64_t>(value);
        else
            static_assert(sizeof(T) == 0, "Unsupported type for ConfigField<T>");
    }

    template <typename T> inline void ConfigField<T>::addSkeleton(js::object &obj) const { obj[name] = skeletonType; }

    template <typename T>
    inline ConfigField<T>::ConfigField(std::string n, T v, std::string t, Config *parent)
        : name(std::move(n)), value(std::move(v)), skeletonType(std::move(t))
    {
        if (parent) parent->childs.push_back(this);
    }

    template <typename T> inline void ConfigArray<T>::load(js::object const &obj)
    {
        items.clear();
        auto *pv = obj.if_contains(name);
        if (!pv) return;
        auto *pa = pv->if_array();
        if (!pa) return;
        if constexpr (JsonObjectElement<T>) {
            for (auto const &j : *pa) {
                auto *po = j.if_object();
                if (!po) continue;
                T item{};
                item.load(*po);
                items.push_back(std::move(item));
            }
            return;
        }
        for (auto const &j : *pa) {
            T v{};
            if (from_json_scalar<T>(j, v)) items.push_back(std::move(v));
        }
    }

    template <typename T> inline void ConfigArray<T>::save(js::object &obj) const
    {
        js::array arr;
        arr.reserve(items.size());
        if constexpr (JsonObjectElement<T>) {
            for (auto const &it : items) {
                js::object elem;
                it.save(elem);
                arr.emplace_back(std::move(elem));
            }
        } else {
            for (auto const &v : items) arr.emplace_back(to_json_scalar<T>(v));
        }
        obj[name] = std::move(arr);
    }

    template <typename T> inline void ConfigArray<T>::addSkeleton(js::object &obj) const
    {
        if constexpr (JsonObjectElement<T>) {
            js::object elem;
            T tmp{};
            tmp.addSkeleton(elem);
            obj[name] = js::array{std::move(elem)};
            return;
        }
        obj[name] = js::array{to_json_scalar<T>(T{})};
    }

    template <typename T> inline ConfigArray<T>::ConfigArray(std::string name_, Config *parent) : name(std::move(name_))
    {
        if (parent) parent->childs.push_back(this);
    }
}