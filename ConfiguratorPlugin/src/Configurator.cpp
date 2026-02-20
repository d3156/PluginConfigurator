#include "Configurator.hpp"
#include <PluginCore/Logger/Log>
#include <linux/prctl.h>
#include <sys/prctl.h>
#include <MetricsModel/MetricsModel>

extern const char *embedded_editor_page;
extern const char *embedded_html_page;
extern const char *embedded_login_page;

void Configurator::registerArgs(d3156::Args::Builder &bldr)
{
    bldr.setVersion(FULL_NAME).addOption(port, "ConfiguratorPort", "Configurator web-server Port");
}

void Configurator::registerModels(d3156::PluginCore::ModelsStorage &models)
{
    model = models.registerModel<ConfiguratorModel>();
    model->registerConfig("MetricsModel", models.registerModel<MetricsModel>()->config);
}

void Configurator::postInit()
{
    thread_ = boost::thread([this]() { this->runIO(); });
}

// ABI required by d3156::PluginCore::Core (dlsym uses exact names)
extern "C" d3156::PluginCore::IPlugin *create_plugin() { return new Configurator(); }

extern "C" void destroy_plugin(d3156::PluginCore::IPlugin *p) { delete p; }

void Configurator::runIO()
{
    prctl(PR_SET_NAME, "ConfiguratorPlugin", 0, 0, 0);
    server = std::make_unique<d3156::EasyWebServer>(io, port);
    G_LOG(0, "Configurator server started at http://0.0.0.0:" << port << "/index.html");
    server->addPath("/reload", [this](const d3156::string_req &req, const d3156::address &a) -> d3156::Answer {
        server->setContentType("text/html; charset=utf-8");
        if (!auth.check(req)) return {true, std::string(embedded_login_page)};
        server->setContentType("text/plain; charset=utf-8");
        raise(SIGINT);
        return {true, "OK"};
    });
    for (auto &conf : model->configsPaths()) {
        server->addPath("/config/" + conf + "/current",
                        [this, conf](const d3156::string_req &req, const d3156::address &a) -> d3156::Answer {
                            server->setContentType("text/html; charset=utf-8");
                            if (!auth.check(req)) return {true, std::string(embedded_login_page)};
                            if (req.method() == d3156::http::verb::post) {
                                model->setCurrent(conf, req.body());
                                server->setContentType("text/plain; charset=utf-8");
                                return {true, "OK"};
                            }
                            server->setContentType("application/json; charset=utf-8");
                            return {true, model->getCurrent(conf)};
                        });
        server->addPath("/config/" + conf + "/sheme",
                        [this, conf](const d3156::string_req &req, const d3156::address &a) -> d3156::Answer {
                            server->setContentType("text/html; charset=utf-8");
                            if (!auth.check(req)) return {true, std::string(embedded_login_page)};
                            server->setContentType("application/json; charset=utf-8");
                            return {true, model->getSheme(conf)};
                        });
    }
    server->addPath("/configs", [this](const d3156::string_req &req, const d3156::address &a) -> d3156::Answer {
        server->setContentType("text/html; charset=utf-8");
        if (!auth.check(req)) return {true, std::string(embedded_login_page)};
        server->setContentType("application/json; charset=utf-8");
        return {true, this->model->configsString()};
    });
    server->addPath("/index.html", [this](const d3156::string_req &req, const d3156::address &a) -> d3156::Answer {
        server->setContentType("text/html; charset=utf-8");
        return {true, std::string(auth.check(req) ? embedded_html_page : embedded_login_page)};
    });
    server->addPath("/editor.bundle.js",
                    [this](const d3156::string_req &req, const d3156::address &a) -> d3156::Answer {
                        server->setContentType("text/html; charset=utf-8");
                        if (!auth.check(req)) return {true, std::string(embedded_login_page)};
                        server->setContentType("application/javascript; charset=utf-8");
                        return {true, embedded_editor_page};
                    });
    io.run();
    G_LOG(1, "Io-context canceled");
}

Configurator::~Configurator()
{
    constexpr boost::chrono::milliseconds stopThreadTimeout = boost::chrono::milliseconds(200);
    try {
        stopToken = true;
        io_guard.reset();
        G_LOG(1, "Io-context guard canceled");
        if (!thread_.joinable()) return;
        G_LOG(1, "Thread joinable, try join in " << stopThreadTimeout.count() << " milliseconds");
        if (thread_.timed_join(stopThreadTimeout)) return;
        Y_LOG(1, "Thread was not terminated, attempting to force stop io_context...");
        io.stop();
        if (thread_.timed_join(stopThreadTimeout)) {
            G_LOG(1, "io_context force stopped successfully");
            return;
        }
        R_LOG(1, "WARNING: Thread cannot be stopped. Thread will be detached (potential resource leak)");
        thread_.detach();
    } catch (std::exception &e) {
        R_LOG(1, "Exception throwed in exit: " << e.what());
    }
}
