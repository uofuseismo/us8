#ifndef US8_MESSAGING_ZEROMQ_AUTHENTICATION_SERVICE_HPP
#define US8_MESSAGING_ZEROMQ_AUTHENTICATION_SERVICE_HPP
#include <us8/messaging/zeromq/authentication/grasslands.hpp>
#include <memory>
namespace US8::Messaging::ZeroMQ::Authentication
{
class Service
{
public:
    /// @brief Constructs a Grasslands authenticator service.
    /// @param[in] context     The context of the socket on which to set the
    ///                        authentication.
    explicit Service(std::shared_ptr<zmq::context_t> context);
    /// @brief Constructs a Grasslands authenticator service.
    /// @param[in] context     The context of the socket on which to set the
    ///                        authentication.
    /// @param[in] grasslands  A grasslands authenticator which allows
    ///                        everything.  Note, the memory from grasslands
    ///                        is moved onto this context.
    Service(std::shared_ptr<zmq::context_t> context,
            Grasslands &&grasslands);

    /// @brief Determines if the user presenting the given username and
    ///        password is valid.
    /// @result A corresponding message to return to the user.
    /// @throws An exception of badrequest, unauthorized, forbidden, or
    ///         serveerror.
/*
    [[nodiscard]] std::string authenticate(
        const US8::Messaging::Authentication::Credential::UserNameAndPassword &credentials) const override final;
    /// @brief Determines if the user presenting the given key is are
    ///        valid.
    /// @result A corresponding message to return.
    /// @throws An exception of badrequest, unauthorized, forbidden, or
    ///         serveerror.
    [[nodiscard]] std::string authenticate(
        const US8::Messaging::Authentication::Credential::KeyPair &keys) const override final;
*/
    /// @brief Starts the authenticator service.
    void start();
    /// @brief Stops the authenticator service.
    void stop();

    ~Service();

    Service(const Service &) = delete;
    Service& operator=(const Service &) = delete;
    
private:
    class ServiceImpl;
    std::unique_ptr<ServiceImpl> pImpl;
};
}
#endif
