#pragma once

#include <boost/json.hpp>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <type_traits>

namespace d3156
{
    namespace js = boost::json;

    struct IConfig {
        virtual void addSkeleton(js::object &obj) const = 0;
        virtual void load(const js::object &obj)        = 0;
        virtual void save(js::object &obj) const        = 0;
        virtual ~IConfig()                              = default;
    };

    struct Config : public IConfig {
        explicit Config(std::string n, Config *parent = nullptr);

        void load(const js::object &root) override;
        void save(js::object &root) const override;
        void addSkeleton(js::object &root) const override;

        std::vector<IConfig *> childs;
        std::string name;
    };

    template <typename T> struct ConfigField : public IConfig {
        std::string name;
        T value;
        std::string skeletonType;

        ConfigField(std::string n, T v, std::string t, Config *parent = nullptr);

        void addSkeleton(js::object &obj) const override;
        void save(js::object &obj) const override;
        void load(const js::object &obj) override;
        operator T &();
    };

    using ConfigString = ConfigField<std::string>;
    using ConfigBool   = ConfigField<bool>;
    using ConfigUint   = ConfigField<std::size_t>;
    using ConfigEnum   = ConfigField<std::string>;

    template <class U>
    concept JsonObjectElement = requires(U u, js::object &o, js::object const &ci) {
        { u.addSkeleton(o) } -> std::same_as<void>;
        { std::as_const(u).save(o) } -> std::same_as<void>;
        { u.load(ci) } -> std::same_as<void>;
    };

    template <class U> using decayed_t = std::remove_cv_t<std::remove_reference_t<U>>;

    template <class U> js::value to_json_scalar(U const &v)
    {
        if constexpr (std::is_same_v<U, std::string>)
            return js::value(v);
        else if constexpr (std::is_same_v<U, bool>)
            return js::value(v);
        else if constexpr (std::is_integral_v<U> && std::is_unsigned_v<U>)
            return js::value(static_cast<std::uint64_t>(v));
        else if constexpr (std::is_integral_v<U> && std::is_signed_v<U>)
            return js::value(static_cast<std::int64_t>(v));
        else
            static_assert(sizeof(U) == 0, "Unsupported scalar type");
    }

    template <class U> bool from_json_scalar(js::value const &j, U &out)
    {
        if constexpr (std::is_same_v<U, std::string>) {
            if (auto *ps = j.if_string()) {
                out = ps->c_str();
                return true;
            }
            return false;
        } else if constexpr (std::is_same_v<U, bool>) {
            if (j.is_bool()) {
                out = j.as_bool();
                return true;
            }
            return false;
        } else if constexpr (std::is_integral_v<U> && std::is_unsigned_v<U>) {
            if (j.is_uint64()) {
                out = static_cast<U>(j.as_uint64());
                return true;
            }
            if (j.is_int64() && j.as_int64() >= 0) {
                out = static_cast<U>(j.as_int64());
                return true;
            }
            return false;
        } else if constexpr (std::is_integral_v<U> && std::is_signed_v<U>) {
            if (j.is_int64()) {
                out = static_cast<U>(j.as_int64());
                return true;
            }
            if (j.is_uint64()) {
                out = static_cast<U>(j.as_uint64());
                return true;
            }
            return false;
        } else
            static_assert(sizeof(U) == 0, "Unsupported scalar type");
    }

    template <typename T> struct ConfigArray : public IConfig {
        std::vector<T> items;
        std::string name;

        ConfigArray(std::string name_, Config *parent = nullptr);

        void addSkeleton(js::object &obj) const override;
        void save(js::object &obj) const override;
        void load(js::object const &obj) override;
    };
}

#define CONFIG_STRING(NAME, val) d3156::ConfigField<std::string> NAME = {#NAME, val, "string", this}
#define CONFIG_BOOL(NAME, val) d3156::ConfigField<bool> NAME = {#NAME, val, "bool", this}
#define CONFIG_UINT(NAME, val) d3156::ConfigField<std::size_t> NAME = {#NAME, val, "uint", this}
#define CONFIG_ENUM(NAME, val, vals) d3156::ConfigField<std::string> NAME = {#NAME, val, vals, this}
#define CONFIG_ARRAY(NAME, T) d3156::ConfigArray<T> NAME = {#NAME, this}

#include "BaseConfigPrivate.hpp"