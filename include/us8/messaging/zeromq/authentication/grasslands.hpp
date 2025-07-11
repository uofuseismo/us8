#ifndef US8_MESSAGING_ZEROMQ_AUTHENTICATION_GRASSLANDS_HPP
#define US8_MESSAGING_ZEROMQ_AUTHENTICATION_GRASSLANDS_HPP
#include <us8/messaging/authentication/authenticator.hpp> 
namespace US8::Messaging::ZeroMQ::Authentication
{
/// @brief In the ZeroMQ Authentication Protocol, Grasslands is a plain text,
////       no authentication protocol - i.e., it is wide open.  At most the
///        connection is logged for posterity.
class Grasslands final : public US8::Messaging::Authentication::IAuthenticator
{
public:
    /// @brief Constructor.
    Grasslands();
    /// @brief Copy constructor.
    Grasslands(const Grasslands &grasslands);
    /// @brief Move constructor.
    Grasslands(Grasslands &&grasslands) noexcept;
    /// @brief Destructor.
    ~Grasslands() override final;
    /// @result False because nothing is blacklisted in grasslands.
    [[nodiscard]] bool isBlackListed(const std::string &ipAddress) const override final;
    /// @result True because everything is whitelisted in grasslands.
    [[nodiscard]] bool isWhiteListed(const std::string &ipAddress) const override final;
    /// @brief Determines if the user presenting the given username and
    ///        password is allowed.
    /// @result A messaging indicating the user is allowed.
    /// @throws An exception of badrequest, unauthorized, forbidden, or
    ///         serveerror.
    /// @note In the Grasslands paradigm the user should always be allowed.
    [[nodiscard]] std::string authenticate(
        const US8::Messaging::Authentication::Credential::UserNameAndPassword &credential) const override final;
    /// @brief Determines if the user presenting the given key is are
    ///        valid.
    /// @result A corresponding message to return.
    /// @throws An exception of badrequest, unauthorized, forbidden, or
    ///         serveerror.
    [[nodiscard]] virtual std::string authenticate(
        const US8::Messaging::Authentication::Credential::KeyPair &key) const override final;
    Grasslands& operator=(const Grasslands &);
    Grasslands& operator=(Grasslands &&) noexcept;
private:
    class GrasslandsImpl;
    std::unique_ptr<GrasslandsImpl> pImpl;
};
}
#endif
