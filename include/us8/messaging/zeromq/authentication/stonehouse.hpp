#ifndef US8_MESSAGING_ZEROMQ_AUTHENTICATION_STONEHOUSE_HPP
#define US8_MESSAGING_ZEROMQ_AUTHENTICATION_STONEHOUSE_HPP
#include <us8/messaging/authentication/authenticator.hpp> 
#include <us8/messaging/authentication/credential/keyPair.hpp>
namespace US8::Messaging::ZeroMQ::Authentication
{
/// @brief In the ZeroMQ Authentication Protocol, Stonehouse is a key exchange,
///        where the client submits its public key to the server, and the
///        server validates that key.
class Stonehouse final : public US8::Messaging::Authentication::IAuthenticator
{
public:
    /// @brief Constructor.
    Stonehouse();
    /// @brief Copy constructor.
    Stonehouse(const Grasslands &grasslands);
    /// @brief Move constructor.
    Stonehouse(Grasslands &&grasslands) noexcept;
    /// @brief Destructor.
    ~Stonehouse() override final;
    /// @throws An exception if the user is blacklisted.
    void blackListed(const std::string &ipAddress) const override final;
    /// @result An exception if the user is not white listed.
    void whiteListed(const std::string &ipAddress) const override final;
    /// @brief Determines if the user presenting the given username and
    ///        password is allowed.
    /// @throws An exception of badrequest since this uses keys.
    [[nodiscard]] std::string authenticate(
        const US8::Messaging::Authentication::Credential::UserNameAndPassword &credential) const override final;
    /// @brief Determines if the user presenting the given key is are
    ///        valid.
    /// @throws An exception of badrequest, unauthorized, forbidden, or
    ///         serveerror.
    [[nodiscard]] virtual std::string authenticate(
        const US8::Messaging::Authentication::Credential::KeyPair &key) const override final;
    Stonehouse& operator=(const Stonehouse &); 
    Stonehouse& operator=(Stonehouse &&) noexcept;
};
}
#endif
