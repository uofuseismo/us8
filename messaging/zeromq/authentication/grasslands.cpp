#include <spdlog/spdlog.h>
#include "us8/messaging/zeromq/authentication/grasslands.hpp"
#include "us8/messaging/authentication/credential/keyPair.hpp"
#include "us8/messaging/authentication/credential/userNameAndPassword.hpp"

#define OKAY_MESSAGE "OK"

using namespace US8::Messaging::ZeroMQ::Authentication;

class Grasslands::GrasslandsImpl
{
public:
};

/// Constructor
Grasslands::Grasslands() :
    pImpl(std::make_unique<GrasslandsImpl> ())
{
}

/// Copy constructor
Grasslands::Grasslands(const Grasslands &grasslands)
{
    *this = grasslands;
}

/// Move construtor
Grasslands::Grasslands(Grasslands &&grasslands) noexcept
{
    *this = std::move(grasslands);
}

/// Copy assignment
Grasslands& Grasslands::operator=(const Grasslands &grasslands)
{
    if (&grasslands == this){return *this;}
    pImpl = std::make_unique<GrasslandsImpl> (*grasslands.pImpl);
    return *this;
}

/// Move constructor
Grasslands& Grasslands::operator=(Grasslands &&grasslands) noexcept
{
    if (&grasslands == this){return *this;}
    pImpl = std::move(grasslands.pImpl);
    return *this;
}

/// Destructor
Grasslands::~Grasslands() = default;

/// Everything comes through!
bool Grasslands::isBlackListed(const std::string &address) const
{
    if (spdlog::get_level() < spdlog::level::debug)
    {
        spdlog::info("Grasslands address: "
                   + address + " is not blacklisted");
    }
    return false;
}   

bool Grasslands::isWhiteListed(const std::string &address) const
{
    if (spdlog::get_level() < spdlog::level::debug)
    {
        spdlog::info("Grasslands address: "
                   + address + " is whitelisted");
    }
    return true;
}

std::string Grasslands::authenticate(
    const US8::Messaging::Authentication::Credential::UserNameAndPassword
    &userNameAndPassword) const
{
    if (spdlog::get_level() < spdlog::level::debug)
    {
        try
        {
            auto name = userNameAndPassword.getUserName();
            spdlog::info("User: " + name + " is allowed");
        }
        catch (...)
        {
            spdlog::info("Grasslands unspecified user is allowed");
        }
    }
    return OKAY_MESSAGE;
}

std::string Grasslands::authenticate(
    const US8::Messaging::Authentication::Credential::KeyPair &) const
{
    if (spdlog::get_level() < spdlog::level::debug)
    {   
        spdlog::info("Grasslands allowing key");
    }   
    return OKAY_MESSAGE;
}

/*
std::pair<std::string, std::string> Grasslands::isValid(
    const Certificate::Keys &) const noexcept
{
    if (spdlog::get_level() < spdlog::level::debug)
    {
        spdlog::info("Grasslands user public key is allowed");
    }
    return OKAY_MESSAGE;
}   


US8::Messaging::Authorization::UserPrivileges 
Grasslands::getMinimumUserPrivileges() const noexcept
{
    return pImpl->mPrivileges;
}
*/
