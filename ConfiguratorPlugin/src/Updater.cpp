#include "Updater.hpp"
#include <boost/algorithm/string/predicate.hpp>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>

#include <boost/json/array.hpp>

#include <boost/url/url_view.hpp>

#include <boost/url/parse.hpp>
#include <chrono>
#include <filesystem>
#include <PluginCore/Logger/Log>
#include <fstream>
#include <ConfiguratorModel>

ConfiguratorExtended::PluginInfo::PluginInfo() : d3156::Config("") {}

ConfiguratorExtended::Updater::Config::Config() : d3156::Config("") {}

ConfiguratorExtended::Updater::Updater(boost::asio::io_context &io) : io_(io), timer_(io_) {}

void ConfiguratorExtended::Updater::init()
{
    if (config.auto_update.value) {
        timer_.expires_after(std::chrono::minutes(config.check_interval_min.value));
        timer_.async_wait([this](const boost::system::error_code &ec) {
            if (!ec) boost::asio::co_spawn(io_, this->check(), boost::asio::detached);
        });
    }
}

boost::asio::awaitable<void> ConfiguratorExtended::Updater::check()
{
    for (auto &i : config.plugins.items) co_await GitLabReleaseLoader(*i, io_).update();
    save();
    timer_.expires_after(std::chrono::minutes(config.check_interval_min.value));
    timer_.async_wait([this](const boost::system::error_code &ec) {
        if (!ec) boost::asio::co_spawn(io_, this->check(), boost::asio::detached);
    });
}

ConfiguratorExtended::Updater::~Updater() { timer_.cancel(); }

ConfiguratorExtended::GitLabReleaseLoader::GitLabReleaseLoader(PluginInfo &info, boost::asio::io_context &io)
    : info_(info), io_(io)
{
    auto r = boost::urls::parse_uri(info.url.value);
    if (!r) throw std::invalid_argument("Invalid URL");
    boost::urls::url_view u = r.value();
    if (u.scheme() != "https" && u.scheme() != "http")
        throw std::invalid_argument("Only http/https URLs are supported");
    auto base = std::string(u.scheme()) + "://" + std::string(u.host());
    if (u.has_port()) base += ":" + std::string(u.port());
    client_          = std::make_unique<d3156::AsyncHttpClient>(io_, base, "", "Bearer " + info.gitlab_token.value);
    std::string path = std::string(u.path());
    while (!path.empty() && path.front() == '/') path.erase(path.begin());
    if (path.empty()) throw std::invalid_argument("URL path is empty");
    if (auto pos = path.find("/-/"); pos != std::string::npos) path.erase(pos);
    if (boost::algorithm::ends_with(path, ".git")) path.resize(path.size() - 4);
    if (path.empty()) throw std::invalid_argument("Project path is empty after normalization");
    project_path = path;
    for (size_t pos = 0; (pos = project_path.find('/', pos)) != std::string::npos;) {
        project_path.replace(pos, 1, "%2F");
        pos += 3;
    }
}

boost::asio::awaitable<bool> ConfiguratorExtended::GitLabReleaseLoader::update()
{
    boost::beast::http::message<
        false, boost::beast::http::basic_dynamic_body<boost::beast::basic_multi_buffer<std::allocator<char>>>>
        redirect = co_await client_->getAsync("/api/v4/projects/" + project_path + "/releases/permalink/latest", "", 2, std::chrono::seconds{2});

    auto loc_it = redirect.base().find(boost::beast::http::field::location);
    if (loc_it == redirect.base().end()) {
        R_LOG(0, "Path to /api/v4/projects/" << project_path
                                             << "/releases/permalink/latest must redirect to latest release");
        co_return false;
    }
    std::string location = std::string(loc_it->value());
    auto body            = beast::buffers_to_string((co_await client_->getAsync(location, "", 2, std::chrono::seconds{2})).body().data());
    try {
        boost::json::value jv = boost::json::parse(body);
        std::string tag_name(jv.at_pointer("/tag_name").as_string());
        if (tag_name == info_.current_tag.value) co_return false;
        for (auto asset : jv.at_pointer("/assets/links").as_array()) {
            auto asset_url = std::string(asset.at_pointer("/direct_asset_url").as_string());
            if (asset_url.find(ARCH_FROM_CI) == std::string::npos) continue;
            std::string target_path = "./Plugins/lib" + info_.plugin_name.value + ".so_" + ARCH_FROM_CI;
            if (co_await d3156::AsyncHttpClient::wget(
                    io_, asset_url, "./Plugins/lib" + info_.plugin_name.value + ".so_" + ARCH_FROM_CI)) {
                std::error_code ec;
                std::filesystem::rename("./Plugins/lib" + info_.plugin_name.value + ".so_" + ARCH_FROM_CI,
                                        "./Plugins/lib" + info_.plugin_name.value + ".so", ec);
                if (!ec) {
                    info_.current_tag.value = tag_name;
                    co_return true;
                }
                R_LOG(1, "Error to atomic move " << ec.message());
            }
        }
    } catch (std::exception &e) {
        R_LOG(1, "Error parse responce by " << "/api/v4/projects/" + project_path << " error: " << e.what());
        R_LOG(1, "Responce: " << body);
    }
    co_return false;
}

boost::asio::awaitable<boost::json::object> ConfiguratorExtended::GitLabReleaseLoader::updateInfo()
{
    boost::beast::http::message<
        false, boost::beast::http::basic_dynamic_body<boost::beast::basic_multi_buffer<std::allocator<char>>>>
        redirect = co_await client_->getAsync("/api/v4/projects/" + project_path + "/releases/permalink/latest", "",2, std::chrono::seconds{2});
    auto loc_it  = redirect.base().find(boost::beast::http::field::location);
    if (loc_it == redirect.base().end()) {
        R_LOG(0, "Path to /api/v4/projects/" << project_path
                                             << "/releases/permalink/latest must redirect to latest release");
        co_return boost::json::object{{"name", info_.plugin_name.value}, {"current_tag", info_.current_tag.value}};
    }
    std::string location = std::string(loc_it->value());
    auto body            = beast::buffers_to_string((co_await client_->getAsync(location, "", 2, std::chrono::seconds{2})).body().data());
    try {
        boost::json::value jv = boost::json::parse(body);
        std::string tag_name(jv.at_pointer("/tag_name").as_string());
        if (tag_name == info_.current_tag.value)
            co_return boost::json::object{{"name", info_.plugin_name.value}, {"current_tag", info_.current_tag.value}};
        bool has_release = false;
        for (auto asset : jv.at_pointer("/assets/links").as_array())
            if (std::string(asset.at_pointer("/direct_asset_url").as_string()).find(ARCH_FROM_CI) !=
                std::string::npos) {
                has_release = true;
                break;
            }
        if (has_release)
            co_return boost::json::object{{"name", info_.plugin_name.value},
                                          {"current_tag", info_.current_tag.value},
                                          {"new_tag", tag_name},
                                          {"description", jv.at_pointer("/description").as_string()}};
    } catch (std::exception &e) {
        R_LOG(1, "Error parse responce by " << "/api/v4/projects/" + project_path << " error: " << e.what());
        R_LOG(1, "Responce: " << body);
    }
    co_return boost::json::object{{"name", info_.plugin_name.value}, {"current_tag", info_.current_tag.value}};
}

boost::asio::awaitable<bool> ConfiguratorExtended::Updater::updateTargeted(std::string name)
{
    for (auto &i : config.plugins.items)
        if (i->plugin_name.value == name)
            if (co_await GitLabReleaseLoader(*i, io_).update()) {
                save();
                co_return true;
            }
    co_return false;
}

boost::asio::awaitable<boost::json::array> ConfiguratorExtended::Updater::updateInfo()
{
    boost::json::array res;
    for (auto &i : config.plugins.items) res.emplace_back(co_await GitLabReleaseLoader(*i, io_).updateInfo());
    co_return res;
}

void ConfiguratorExtended::Updater::save()
{
    d3156::js::object obj;
    config.save(obj);
    std::ofstream ofs("./configs/ConfiguratorUpdater.json", std::ios::binary | std::ios::trunc);
    if (!ofs) throw std::runtime_error("Cannot open config for write: ./configs/ConfiguratorUpdater.json");
    ConfiguratorModel::pretty_print(ofs, d3156::js::value(std::move(obj)));
}
