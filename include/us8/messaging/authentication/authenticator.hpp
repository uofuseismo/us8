#ifndef US8_MESSAGING_AUTHENTICATION_AUTHENTICATOR_HPP
#define US8_MESSAGING_AUTHENTICATION_AUTHENTICATOR_HPP
#include <string>
#include <us8/messaging/authentication/exceptions.hpp>
namespace US8::Messaging::Authentication::Credential
{
 class KeyPair;
 class UserNameAndPassword;
}
namespace US8::Messaging::Authentication
{
/// @class IAuthenticator "authenticator.hpp" "us8/messaging/authentication/authenticator.hpp"
/// @brief This the base class that defines an authenticator.  
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class IAuthenticator
{
public:
    /// @brief Defines an access role. 
    enum class Role : int8_t
    {
        ReadOnly = 0, /*!< A client can only read. */
        ReadWrite = 1, /*!< A client can read and write. */
        Administrator = 1 /*!< An extremely privileged client. */
    };
public:
    /// @throws An exeption of forbidden if the IP is not allowed.
    virtual void blackListed(const std::string &address) const = 0;
    /// @result OK indicates the given IP address is whitelisted and should
    ///         be allowed access.
    /// @throws An exeption of forbidden if the IP is not allowed.
    virtual void whiteListed(const std::string &address) const = 0;
    /// @result The minimum user privileges for this authenticator.
    //[[nodiscard]] virtual Privileges getMinimumUserPrivileges() const noexcept = 0;
    /// @brief Determines if the user presenting the given username and
    ///        password is valid.
    /// @result A corresponding message to return to the user.
    /// @throws An exception of badrequest, unauthorized, forbidden, or
    ///         serveerror.
    [[nodiscard]] virtual std::string authenticate(
        const US8::Messaging::Authentication::Credential::UserNameAndPassword &credentials) const = 0;
    /// @brief Determines if the user presenting the given key is are
    ///        valid.
    /// @result A corresponding message to return.
    /// @throws An exception of badrequest, unauthorized, forbidden, or
    ///         serveerror.
    [[nodiscard]] virtual std::string authenticate(
        const US8::Messaging::Authentication::Credential::KeyPair &keys) const = 0;
    /// @brief Determines if the user presenting the given username and
    ///        password is valid.
    /// @result A corresponding message to return to the user.
    /// @throws An exception of badrequest, unauthorized, forbidden, or
    ///         serveerror.
    [[nodiscard]] virtual std::string operator()(const US8::Messaging::Authentication::Credential::UserNameAndPassword &credentials) const;
    /// @brief Determines if the user presenting the given key is are
    ///        valid.
    /// @result A corresponding message to return.
    /// @throws An exception of badrequest, unauthorized, forbidden, or
    ///         serveerror.
    [[nodiscard]] virtual std::string operator()(const US8::Messaging::Authentication::Credential::KeyPair &credentials) const;

    /// @brief Destructor
    virtual ~IAuthenticator();
};
}
#endif
