#include "crypto.hpp"

#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <string>

std::string sha256(const std::string& str)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    // TODO: Use EVP instead
    if (!SHA256_Init(&sha256)) {
        return std::string{};
    }

    if (!SHA256_Update(&sha256, str.c_str(), str.size())) {
        return std::string{};
    }

    if (!SHA256_Final(hash, &sha256)) {
        return std::string{};
    }

    std::stringstream ss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}