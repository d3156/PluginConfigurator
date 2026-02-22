#pragma once
#include <BaseConfig>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/awaitable.hpp>
#include <memory>
#include <string>
#include <EasyHttpLib/AsyncHttpClient>

namespace ConfiguratorExtended
{
    struct PluginInfo : public d3156::Config {
        PluginInfo();
        CONFIG_STRING(plugin_name, "");
        CONFIG_STRING(current_tag, "");
        CONFIG_STRING(url, "");
        CONFIG_STRING(gitlab_token, "");
    };

    class GitLabReleaseLoader
    {
        std::string project_path;
        boost::asio::io_context &io_;
        PluginInfo & info_;
        std::unique_ptr<d3156::AsyncHttpClient> client_;

    public:
        GitLabReleaseLoader(PluginInfo &, boost::asio::io_context &io);
        boost::asio::awaitable<bool> update();
        boost::asio::awaitable<boost::json::object> updateInfo();
    };

    class Updater
    {
        boost::asio::io_context &io_;
        boost::asio::steady_timer timer_;

    public:
        Updater(boost::asio::io_context &io);
        void init();
        ~Updater();

        struct Config : public d3156::Config {
            Config();
            CONFIG_BOOL(auto_update, false);
            CONFIG_UINT(check_interval_min, 30);
            CONFIG_BOOL(enable_web, false);
            CONFIG_ARRAY(plugins, PluginInfo);
        } config;

        void save();

        boost::asio::awaitable<void> check();
        boost::asio::awaitable<boost::json::array> updateInfo();
        boost::asio::awaitable<bool> updateTargeted(std::string name);
    };
}
