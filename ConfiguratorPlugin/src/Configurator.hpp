#include <PluginCore/IPlugin>
#include <PluginCore/IModel>
#include <boost/asio/io_context.hpp>
#include <boost/thread.hpp>
#include <string>
#include <ConfiguratorModel>
#include <EasyHttpLib/EasyWebServer>

class Configurator final : public d3156::PluginCore::IPlugin
{
    std::string configPath;
    boost::asio::io_context io;
    boost::thread thread_;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> io_guard =
        boost::asio::make_work_guard(io);
    std::atomic<bool> stopToken = false;
    uint16_t port               = 5569;
    std::unique_ptr<d3156::EasyWebServer> server;
    ConfiguratorModel *model;

public:
    void runIO();
    void registerArgs(d3156::Args::Builder &bldr) override;
    void registerModels(d3156::PluginCore::ModelsStorage &models) override;
    void postInit() override;

    ~Configurator();
};
