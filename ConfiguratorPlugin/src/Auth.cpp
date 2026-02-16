#include "Auth.hpp"
#include <fstream>
#include <filesystem>
#include <PluginCore/Logger/Log>
#include <openssl/hmac.h>
#include <openssl/rand.h>
namespace d3156
{
    std::string sha256(const std::string &input)
    {
        unsigned char hash[32]; // SHA256 = 32 байта
        if (!EVP_Digest(input.data(), input.size(), hash, nullptr, EVP_sha256(), nullptr)) {
            R_LOG(0, "SHA256 computation failed");
            return "";
        }
        std::stringstream ss;
        for (int i = 0; i < 32; ++i) ss << std::hex << std::setw(2) << std::setfill('0') << (unsigned)hash[i];
        return ss.str();
    }

    static std::string generateTOTPForCounter(const std::vector<uint8_t> &secret, uint64_t counter)
    {
        constexpr int digits = 6;
        uint8_t counter_bytes[8];
        for (int i = 7; i >= 0; --i) {
            counter_bytes[i] = counter & 0xFF;
            counter >>= 8;
        }
        unsigned int len = 0;
        unsigned char hmac_result[EVP_MAX_MD_SIZE];
        HMAC(EVP_sha1(), secret.data(), static_cast<int>(secret.size()), counter_bytes, 8, hmac_result, &len);
        int offset      = hmac_result[len - 1] & 0x0F;
        uint32_t binary = ((hmac_result[offset] & 0x7F) << 24) | ((hmac_result[offset + 1] & 0xFF) << 16) |
                          ((hmac_result[offset + 2] & 0xFF) << 8) | (hmac_result[offset + 3] & 0xFF);
        uint32_t otp = binary % 1000000;
        std::ostringstream ss;
        ss << std::setw(digits) << std::setfill('0') << otp;
        return ss.str();
    }

    bool d3156::Auth::check(const string_req &req)
    {
        auto auth_it = req.find(d3156::http::field::authorization);
        if (auth_it == req.end()) return false;
        std::string authorization = auth_it->value();
        if (authorization.empty()) return false;
        auto pos = authorization.find("?");
        if (pos == std::string::npos) return false;
        std::string hash = authorization.substr(0, pos);
        std::string salt = authorization.substr(pos + 1);
        return verifyTOTPWithWindow(hash, salt);
    }

    std::string generateRandomString(int length)
    {
        const std::string CHARACTERS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        std::random_device rd;
        std::mt19937 generator(rd());
        std::uniform_int_distribution<> distribution(0, CHARACTERS.size() - 1);
        std::string random_string;
        for (int i = 0; i < length; ++i) random_string += CHARACTERS[distribution(generator)];
        return random_string;
    }

    Auth::Auth()
    {
        const std::string filepath = "./ConfiguratorSecrets";
        if (std::filesystem::exists(filepath)) {
            std::ifstream ifs(filepath);
            if (!ifs.is_open()) {
                R_LOG(0, "Failed to open existing ConfiguratorSecrets");
                return;
            }
            std::string line;
            while (std::getline(ifs, line)) {
                if (line.find("username:") == 0)
                    username = line.substr(9);
                else if (line.find("totp_secret:") == 0) {
                    std::string hex = line.substr(12);
                    totp_secret.resize(hex.size() / 2);
                    for (size_t i = 0; i < totp_secret.size(); ++i)
                        totp_secret[i] = static_cast<uint8_t>(std::stoi(hex.substr(i * 2, 2), nullptr, 16));
                }
            }
            ifs.close();
            G_LOG(0, "ConfiguratorSecrets loaded from " << filepath);
            return;
        }
        username = generateRandomString(10);
        totp_secret.resize(32);
        if (RAND_bytes(totp_secret.data(), static_cast<int>(totp_secret.size())) != 1) {
            R_LOG(0, "OpenSSL RAND_bytes failed");
            return;
        }
        std::ofstream ofs(filepath, std::ios::out | std::ios::trunc);
        if (!ofs.is_open()) {
            R_LOG(0, "Failed to open " << filepath << " for writing.");
            return;
        }
        ofs << "username:" << username << "\n";
        ofs << "totp_secret:";
        for (auto b : totp_secret) { ofs << std::hex << std::setw(2) << std::setfill('0') << (int)b; }
        ofs << "\n";
        ofs.close();
        G_LOG(0, "ConfiguratorSecrets created successfully in " << filepath);
    }
    
    bool Auth::verifyTOTPWithWindow(const std::string &expectedHash, const std::string &salt)
    {
        constexpr uint64_t timestep = 15;
        uint64_t currentCounter     = std::time(nullptr) / timestep;
        for (int offset = -1; offset <= 1; ++offset) { // Проверяем -1, 0, +1 шаг
            uint64_t counter = currentCounter + offset;
            std::string totp = generateTOTPForCounter(totp_secret, counter);
            if (sha256(username + totp + salt) == expectedHash) return true;
            R_LOG(1, "=== Debug Auth Params ===");
            R_LOG(1, "Username:" << username);
            R_LOG(1, "TOTP:" << totp);
            R_LOG(1, "Salt:" << salt);
            R_LOG(1, "Counter (15s step):" << counter);
            R_LOG(1, "SHA256(username + totp + salt):" << sha256(username + totp + salt));
        }
        return false;
    }
}