cmake_minimum_required(VERSION 3.16)
project(ConfiguratorPlugin VERSION 1.0.0 LANGUAGES CXX)
include("${CMAKE_CURRENT_SOURCE_DIR}/../../../tools/workspace.cmake")
create_target(plugin)
dependency("../.." ConfiguratorModel "https://gitlab.bubki.zip/d3156/PluginConfigurator.git")
dependency("../.." EasyHttpLib "https://gitlab.bubki.zip/d3156/EasyHttpLib.git")
dependency("../.." MetricsModel "https://gitlab.bubki.zip/d3156/MetricsModel.git")

file(READ ${CMAKE_CURRENT_SOURCE_DIR}/web/index.html PAGE_HTML)
string(REPLACE "\\" "\\\\" PAGE_HTML "${PAGE_HTML}")
string(REPLACE "\"" "\\\"" PAGE_HTML "${PAGE_HTML}")
string(REPLACE "\n" "\\n\"\n\"" PAGE_HTML "${PAGE_HTML}")

file(READ ${CMAKE_CURRENT_SOURCE_DIR}/web/login.html LOGIN_HTML)
string(REPLACE "\\" "\\\\" LOGIN_HTML "${LOGIN_HTML}")
string(REPLACE "\"" "\\\"" LOGIN_HTML "${LOGIN_HTML}")
string(REPLACE "\n" "\\n\"\n\"" LOGIN_HTML "${LOGIN_HTML}")

file(READ ${CMAKE_CURRENT_SOURCE_DIR}/web/editor.bundle.js EDITOR_BUNDLE)
string(REPLACE "\\" "\\\\" EDITOR_BUNDLE "${EDITOR_BUNDLE}")
string(REPLACE "\"" "\\\"" EDITOR_BUNDLE "${EDITOR_BUNDLE}")
string(REPLACE "\n" "\\n\"\n\"" EDITOR_BUNDLE "${EDITOR_BUNDLE}")

file(READ ${CMAKE_CURRENT_SOURCE_DIR}/web/logs.html LOGS_HTML)
string(REPLACE "\\" "\\\\" LOGS_HTML "${LOGS_HTML}")
string(REPLACE "\"" "\\\"" LOGS_HTML "${LOGS_HTML}")
string(REPLACE "\n" "\\n\"\n\"" LOGS_HTML "${LOGS_HTML}")

file(READ ${CMAKE_CURRENT_SOURCE_DIR}/web/auth.js AUTH_JS)
string(REPLACE "\\" "\\\\" AUTH_JS "${AUTH_JS}")
string(REPLACE "\"" "\\\"" AUTH_JS "${AUTH_JS}")
string(REPLACE "\n" "\\n\"\n\"" AUTH_JS "${AUTH_JS}")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/web/embedded_page.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/embedded_page.cpp @ONLY)
target_sources(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/embedded_page.cpp)

find_package(OpenSSL REQUIRED COMPONENTS SSL Crypto)
target_link_libraries(ConfiguratorPlugin PUBLIC OpenSSL::SSL OpenSSL::Crypto)

find_package(Boost 1.8 REQUIRED COMPONENTS json thread system)
target_link_libraries(ConfiguratorPlugin PRIVATE Boost::json Boost::system Boost::thread)