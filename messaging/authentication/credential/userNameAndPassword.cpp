#include <array>
#include <string>
#ifndef NDEBUG
#include <cassert>
#endif
#include "us8/messaging/authentication/credential/userNameAndPassword.hpp"

using namespace US8::Messaging::Authentication::Credential;

/*
namespace
{

int hashLevelToIndex(const HashLevel level)
{
    if (level == HashLevel::Interactive){return 0;}
    if (level == HashLevel::Moderate){return 1;}
#ifndef NDEBUG
    assert(level == HashLevel::Sensitive);
#endif
    return 2;
}

/// @brief This is utility for storing a password by first.
/// @param[in] password   The plain text password to convert to a hashed
///                       password.
/// @param[in] opslimit   Limits the operations.  This will take about
///                       3 seconds on 
/// @param[in] opslimit   Controls the max amount of computations performed
///                       by libsodium.
/// @param[in] memlimit   Controls the max amount of RAM libsodium will use.
/// @result The corresponding hashed string to store in a database.
/// @note The default algorithm will take about 3.5 seconds an a 2.8 GHz
///       Core i7 CPU and require ~1 Gb of RAM.
std::string pwhashString(
    const std::string &password,
    unsigned long long opslimit = crypto_pwhash_OPSLIMIT_SENSITIVE,
    unsigned long long memlimit = crypto_pwhash_MEMLIMIT_SENSITIVE)
{
    std::array<char, crypto_pwhash_STRBYTES> work{};
    std::fill(work.begin(), work.end(), '\0');
    auto rc = crypto_pwhash_str(work.data(),
                                password.c_str(), password.size(),
                                opslimit, memlimit);
    if (rc != 0)
    {
        auto errmsg = "Failed to hash string.  Likely hit memory limit";
        throw std::runtime_error(errmsg);
    }
    std::string hashedPassword{work.data()};
    return hashedPassword;
}
}
*/

class UserNameAndPassword::UserNameAndPasswordImpl
{
public:
    std::string mUserName;
    std::string mPassword;
    bool mHavePassword{false};
};

/// Constructor exclusively based on user name
UserNameAndPassword::UserNameAndPassword(const std::string &userName) :
    pImpl(std::make_unique<UserNameAndPasswordImpl> ())
{
    if (userName.empty())
    {
        throw std::invalid_argument("User name is empty");
    }
    pImpl->mUserName = userName;
}

UserNameAndPassword::UserNameAndPassword(const std::string &userName,
                                         const std::string &password) :
    pImpl(std::make_unique<UserNameAndPasswordImpl> ()) 
{
    if (userName.empty())
    {
        throw std::invalid_argument("User name is empty");
    }
    pImpl->mUserName = userName;
    if (!password.empty())
    {
        pImpl->mPassword = password;
        pImpl->mHavePassword = true;
    }
}

/// Copy constructor
UserNameAndPassword::UserNameAndPassword(const UserNameAndPassword &plainText)
{
    *this = plainText;
}

/// Move constructor
UserNameAndPassword::UserNameAndPassword(
    UserNameAndPassword &&plainText) noexcept
{
    *this = std::move(plainText);
}

/// Copy assignment
UserNameAndPassword& UserNameAndPassword::operator=(
    const UserNameAndPassword &plainText)
{
    if (&plainText == this){return *this;}
    pImpl = std::make_unique<UserNameAndPasswordImpl> (*plainText.pImpl);
    return *this;
}

/// Move assignment
UserNameAndPassword& UserNameAndPassword::operator=(
   UserNameAndPassword &&plainText) noexcept
{
    if (&plainText == this){return *this;}
    pImpl = std::move(plainText.pImpl);
    return *this;
}

/// Destructor
UserNameAndPassword::~UserNameAndPassword() = default;

/// Reset class
void UserNameAndPassword::clear() noexcept
{
    pImpl = std::make_unique<UserNameAndPasswordImpl> ();
}

/// User name
std::string UserNameAndPassword::getUserName() const
{
    if (pImpl->mUserName.empty())
    {
        throw std::runtime_error("User name not set");
    }
    return pImpl->mUserName;
}

/// Password
std::optional<std::string> UserNameAndPassword::getPassword() const noexcept
{
    return pImpl->mHavePassword ?
           std::optional<std::string> (pImpl->mPassword) :
           std::nullopt;
}

