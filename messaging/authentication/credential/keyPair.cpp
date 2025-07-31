#include <vector>
#include <optional>
#include <string>
#ifndef NDEBUG
#include <cassert>
#endif
#include "us8/messaging/authentication/credential/keyPair.hpp"

using namespace US8::Messaging::Authentication::Credential;

class KeyPair::KeyPairImpl
{
public:
    std::vector<uint8_t> mPublicKey;
    std::vector<uint8_t> mPrivateKey;
};

KeyPair::KeyPair(const std::vector<uint8_t> &publicKey) :
    pImpl(std::make_unique<KeyPairImpl> ())
{
    if (publicKey.empty())
    {
        throw std::invalid_argument("Public key is empty");
    }
    pImpl->mPublicKey = publicKey;
}

KeyPair::KeyPair(const std::vector<uint8_t> &publicKey,
                 const std::vector<uint8_t> &privateKey) :
    pImpl(std::make_unique<KeyPairImpl> ())
{
    if (publicKey.empty())
    {
        throw std::invalid_argument("Public key is empty");
    }
    pImpl->mPublicKey = publicKey;
    pImpl->mPrivateKey = privateKey;
}

KeyPair::KeyPair(const KeyPair &keyPair)
{
    *this = keyPair;
}

KeyPair::KeyPair(KeyPair &&keyPair) noexcept
{
    *this = std::move(keyPair);
}

KeyPair& KeyPair::operator=(const KeyPair &keyPair)
{
    if (&keyPair == this){return *this;}
    pImpl = std::make_unique<KeyPairImpl> (*keyPair.pImpl);
    return *this;
}

KeyPair& KeyPair::operator=(KeyPair &&keyPair) noexcept
{
    if (&keyPair == this){return *this;}
    pImpl = std::move(keyPair.pImpl);
    return *this;
}

std::vector<uint8_t> KeyPair::getPublicKey() const
{
    if (pImpl->mPublicKey.empty())
    {
        throw std::runtime_error("Public key not set");
    }
    return pImpl->mPublicKey;
}

std::optional<std::vector<uint8_t>> KeyPair::getPrivateKey() const noexcept
{
    return !pImpl->mPrivateKey.empty() ?
           std::optional<std::vector<uint8_t>> (pImpl->mPrivateKey) :
           std::nullopt;
}

KeyPair::~KeyPair() = default;

