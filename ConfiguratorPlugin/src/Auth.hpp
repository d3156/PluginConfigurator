#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <EasyHttpLib/EasyWebServer>

namespace d3156
{

    class Auth
    {
        std::string username;
        std::vector<uint8_t> totp_secret;

    public:
        Auth();
        bool check(const string_req &req);

        bool verifyTOTPWithWindow(const std::string &expectedHash, const std::string &salt);
    };

}
