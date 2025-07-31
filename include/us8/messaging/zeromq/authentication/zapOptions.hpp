#ifndef US8_MESSAGING_ZEROMQ_AUTHENTICATION_ZAP_OPTIONS_HPP
#define US8_MESSAGING_ZEROMQ_AUTHENTICATION_ZAP_OPTIONS_HPP
#include <memory>
// Forward declarations
namespace zmq
{
  class socket_t;
}
namespace US8::Messaging::Authentication::Credential
{
 class UserNameAndPassword;
 class KeyPair;
}
namespace US8::Messaging::ZeroMQ::Authentication
{
/// @brief Defines the authentication protocol.
enum class Protocol
{   
    Grasslands = 0, /*!< No authentication. */
    Strawhouse = 1, /*!< Server authenticates client's IP address. */
    Woodhouse = 2,  /*!< Server authenticates client's IP address, username,
                         and password. */
    Stonehouse = 3, /*!< Server authenticates client's IP address and client's
                         public key. */
    Ironhouse = 4   /*!< Server authenticates client's IP address and client's 
                         public key and client validate's server's public key. */
};  

class IZAPOptions
{
public:
    /// @brief A utility routine that sets the ZAP options on the
    ///        ZeroMQ socket.
    /// @param[in,out] socket  The ZeroMQ socket.  On input, this socket must
    ///                        not be connected when this is called, i.e.,
    ///                        use this method before a zmq_bind or zmq_connect 
    ///                        call.  On exit, the appropriate ZeroMQ ZAP
    ///                        options for the desired security level will be
    ///                        set on the socket.
    /// @throws std::runtime_error if the options cannot be set.
    virtual void setSocketOptions(zmq::socket_t *socket) const = 0;
    /// @result The protocol for this implementation.
    [[nodiscard]] virtual Protocol getProtocol() const noexcept = 0;
    /// @result True indicates this is a server and is responsible for authentication.
    [[nodiscard]] virtual bool isAuthenticationServer() const noexcept = 0;
    /// @brief Destructor
    virtual ~IZAPOptions();
};

class GrasslandsClient final : public IZAPOptions
{
public:
    GrasslandsClient();
    GrasslandsClient(const GrasslandsClient &client);
    GrasslandsClient(GrasslandsClient &&client) noexcept;
    /// @result Set the socket options.
    void setSocketOptions(zmq::socket_t *socket) const final;
    /// @result Grasslands.
    [[nodiscard]] Protocol getProtocol() const noexcept override final;
    /// @result False.
    [[nodiscard]] bool isAuthenticationServer() const noexcept override final;
    /// @brief Destructor.
    ~GrasslandsClient() final;
    /// @brief Copy assignment.
    GrasslandsClient& operator=(const GrasslandsClient &client);
    /// @brief Move assignemnt.
    GrasslandsClient& operator=(GrasslandsClient &&client) noexcept;
private:
    class GrasslandsClientImpl;
    std::unique_ptr<GrasslandsClientImpl> pImpl;
};

class GrasslandsServer final : public IZAPOptions
{   
public:
    GrasslandsServer();
    GrasslandsServer(const GrasslandsServer &server);
    GrasslandsServer(GrasslandsServer &&server) noexcept;
    /// @result Set the socket options.
    void setSocketOptions(zmq::socket_t *socket) const final;
    /// @result Grasslands.
    [[nodiscard]] Protocol getProtocol() const noexcept override final;
    /// @result True, however, no authentication is performed.
    [[nodiscard]] bool isAuthenticationServer() const noexcept override final;
    /// @result The authentication domain.
    [[nodiscard]] std::string getDomain() const noexcept;
    /// @brief Destructor.
    ~GrasslandsServer() final;
    /// @brief Copy assignment.
    GrasslandsServer& operator=(const GrasslandsServer &server);
    /// @brief Move assignemnt.
    GrasslandsServer& operator=(GrasslandsServer &&server) noexcept;
private:
    class GrasslandsServerImpl;
    std::unique_ptr<GrasslandsServerImpl> pImpl;
};

class StrawhouseServer final : public IZAPOptions
{   
public:
    StrawhouseServer();
    StrawhouseServer(const StrawhouseServer &server);
    StrawhouseServer(StrawhouseServer &&server) noexcept;
    /// @result Set the socket options.
    void setSocketOptions(zmq::socket_t *socket) const final;
    /// @result Strawhouse.
    [[nodiscard]] Protocol getProtocol() const noexcept override final;
    /// @result True, however, no authentication is performed.
    [[nodiscard]] bool isAuthenticationServer() const noexcept override final;
    /// @result The authentication domain.
    [[nodiscard]] std::string getDomain() const noexcept;
    /// @brief Destructor.
    ~StrawhouseServer() final;
    /// @brief Copy assignment.
    StrawhouseServer& operator=(const StrawhouseServer &server);
    /// @brief Move assignemnt.
    StrawhouseServer& operator=(StrawhouseServer &&server) noexcept;
private:
    class StrawhouseServerImpl;
    std::unique_ptr<StrawhouseServerImpl> pImpl;
};

class StonehouseClient final : public IZAPOptions
{
public:
    StonehouseClient(const US8::Messaging::Authentication::Credential::KeyPair &clientKeyPair,
                     const US8::Messaging::Authentication::Credential::KeyPair &serverPublicKey);
    StonehouseClient(const StonehouseClient &client);
    StonehouseClient(StonehouseClient &&client) noexcept;
    /// @result Set the socket options.
    void setSocketOptions(zmq::socket_t *socket) const final;
    /// @result Stonehouse.
    [[nodiscard]] Protocol getProtocol() const noexcept override final;
    /// @result False.
    [[nodiscard]] bool isAuthenticationServer() const noexcept override final;
    /// @result The authentication domain.
    [[nodiscard]] std::string getDomain() const noexcept;
    /// @brief Destructor.
    ~StonehouseClient() final;
    /// @brief Copy assignment.
    StonehouseClient& operator=(const StonehouseClient &client);
    /// @brief Move assignemnt.
    StonehouseClient& operator=(StonehouseClient &&client) noexcept;
    StonehouseClient() = delete;
private:
    class StonehouseClientImpl;
    std::unique_ptr<StonehouseClientImpl> pImpl;
};

class StonehouseServer final : public IZAPOptions
{
public:
    explicit StonehouseServer(const US8::Messaging::Authentication::Credential::KeyPair &serverKeyPair);
    StonehouseServer(const StonehouseServer &server);
    StonehouseServer(StonehouseServer &&server) noexcept;
    /// @result Set the socket options.
    void setSocketOptions(zmq::socket_t *socket) const final;
    /// @result Stonehouse.
    [[nodiscard]] Protocol getProtocol() const noexcept override final;
    /// @result True.
    [[nodiscard]] bool isAuthenticationServer() const noexcept override final;
    /// @result The authentication domain.
    [[nodiscard]] std::string getDomain() const noexcept;
    /// @brief Destructor.
    ~StonehouseServer() final;
    /// @brief Copy assignment.
    StonehouseServer& operator=(const StonehouseServer &server);
    /// @brief Move assignemnt.
    StonehouseServer& operator=(StonehouseServer &&server) noexcept;
    StonehouseServer() = delete;
private:
    class StonehouseServerImpl;
    std::unique_ptr<StonehouseServerImpl> pImpl;
};

/// @class ZAPOptions "zapOptions.hpp" "us8/messaging/authentication/zeromq/zapOptions.hpp"
/// @brief Defines options for using the ZeroMQ Authentication Protocol options.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
/*
class ZAPOptions
{
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    ZAPOptions();
    /// @brief Copy assignment operator.
    /// @param[in] options  The options class from which to initialize
    ///                     this class. 
    ZAPOptions(const ZAPOptions &options);
    /// @brief Move assignment operator.
    /// @param[in,out] options  The options class from which to initialize
    ///                         this class.  On exit, options's behavior is
    ///                         undefined.
    ZAPOptions(ZAPOptions &&options) noexcept;

    /// @}

    /// @name Operators
    /// @{

    /// @brief Copy assignment.
    /// @param[in] options   The options to copy to this.
    /// @result A deep copy of the input options.
    ZAPOptions& operator=(const ZAPOptions &options);
    /// @brief Move assignment.
    /// @param[in,out] options  The options class whose memory will be moved
    ///                         to this.  On exit, options's behavior is
    ///                         undefined.
    /// @result The memory from options moved to this.
    ZAPOptions& operator=(ZAPOptions &&options) noexcept;

    /// @}

    /// @name Grasslands
    /// @{

    /// @brief This enables the grasslands security pattern.  Effectively, there
    ///        is no security.  This is the default security pattern.
    void setGrasslandsClient() noexcept;
    /// @brief This enables the grasslands security pattern.  Effectively, there
    ///        is no security.  This is the default security pattern.
    void setGrasslandsServer() noexcept;

    /// @}
    
    /// @name Strawhouse
    /// @{

    /// @brief This enables the strawhouse security pattern for the client.
    ///        In this case nothing else needs to be specified.
    void setStrawhouseClient() noexcept;
    /// @brief This enables the strawhouse security pattern for a ZAP server.
    ///        The server will be provided the client's IP address on 
    ///        \c getDomain() and an auxiliary service will verify.
    void setStrawhouseServer() noexcept;

    /// @}

    /// @name Woodhouse
    /// @{

    /// @brief This enables the woodhouse security pattern for the client.
    ///        Here, the client provides their username and password which
    ///        ZeroMQ will pass to the server during authentication.
    ///        Additionally, the IP may be checked.
    /// @param[in] credentials  The client's name and corresponding password.
    /// @throws std::invalid_argument if credentials.haveName() or 
    ///         credentials.havePassword() is false.
    void setWoodhouseClient(const Messaging::Certificate::UserNameAndPassword &credentials);
    /// @brief This enables the woodhouse security pattern for the server.
    ///        The server will be provided the client's name, password, and IP
    ///        on \c getDomain() then an auxiliary service will verify.
    void setWoodhouseServer() noexcept;
    /// @result The client's username and password.
    [[nodiscard]] Messaging::Certificate::UserNameAndPassword getClientCredentials() const;

    /// @}

    /// @name Stonehouse
    /// @{

    /// @brief This enables the stonehouse security pattern for the client.
    ///        The client must know the server's public key.  Additionally,
    ///        the client must have a valid public and private key pair.
    ///        The public key will be encrypted, sent to the server, and
    ///        validated.  Additionally, the IP may be validated.
    /// @param[in] serverKeys  The server's public key.
    /// @param[in] clientKeys  The client's public and private key pair.
    /// @throws std::invalid_argument if server.havePublicKey() is false,
    ///         client.havePublicKey() is false, or client.havePrivateKey()
    ///         is false. 
    /// @note Upon successful completion, isAuthenticationServer() will
    ///       be false.
    void setStonehouseClient(const Messaging::Certificate::KeyPair &serverKeys,
                             const Messaging::Certificate::KeyPair &clientKeys); 
    /// @brief This enbales the stonehouse security pattern for the server.
    ///        The server will be provided the client's public key and IP
    ///        on \c getDomain() then an auxiliary service will verify.
    /// @param[in] serverKeys  The server's public and private keys.
    /// @throws std::invalid_argument if server.havePublicKey() or 
    ///         \c server.havePrivateKey() is false.
    void setStonehouseServer(const Messaging::Certificate::KeyPair &serverKeys);
    /// @result The server's public and, potentially, private key information.
    [[nodiscard]] Certificate::KeyPair getServerKeyPair() const;
    /// @result The client's public and, potentially, private key information. 
    [[nodiscard]] Certificate::KeyPair getClientKeyPair() const;

    /// @}

    /// @name ZAP Domain
    /// @{

    /// @brief Sets the ZeroMQ Authentication Protocol Domain.  Effectively,
    ///        ZeroMQ will send an inter-process message to the authenticator
    ///        on a connection that utilizes this name.
    /// @param[in] zapDomain  The ZAP domain.
    /// @throws std::invalid_argument if zapDomain is blank.
    /// @note This will only be accessed if \c isAuthenticationServer() is true.
    void setDomain(const std::string &zapDomain);
    /// @result The ZAP domain.  By default this is "global".
    [[nodiscard]] std::string getDomain() const noexcept;

    /// @}

    /// @name Auxiliary Properties
    /// @{

    /// @result The currently set security level.
    /// @note This is automatically set by the most recent call to the
    ///       \c setGrasslandsClient(), \c setGrasslandsServer(),
    ///       \c setStrawhouseClient(), \c setStrawhouseServer(),
    ///       \c setWoodhouseClient(), \c setWoodhouseServer(),
    ///       \c setStonehouseClient(), \c setStonehouseServer().
    [[nodiscard]] SecurityLevel getSecurityLevel() const noexcept;
    /// @result True indicates this is a ZAP authentication server.
    ///         False indicates this is a ZAP authentication client.
    /// @note This is automatically set by the most recent call to the
    ///       \c setGrasslandsClient(), \c setGrasslandsServer(),
    ///       \c setStrawhouseClient(), \c setStrawhouseServer(),
    ///       \c setWoodhouseClient(), \c setWoodhouseServer(),
    ///       \c setStonehouseClient(), \c setStonehouseServer().
    [[nodiscard]] bool isAuthenticationServer() const noexcept;

    /// @}

    /// @brief A utility routine that sets the ZAP options on the
    ///        ZeroMQ socket.
    /// @param[in,out] socket  The ZeroMQ socket.  On input, this socket must
    ///                        not be connected when this is called, i.e.,
    ///                        use this method before a zmq_bind or zmq_connect 
    ///                        call.  On exit, the appropriate ZeroMQ ZAP
    ///                        options for the desired security level will be
    ///                        set on the socket.
    /// @throws std::runtime_error if the options cannot be set.
    //void setSocketOptions(zmq::socket_t *socket) const;

    /// @name Destructors
    /// @{

    /// @brief Resets the class to the grasslands pattern and releases
    ///        all memory.
    void clear() noexcept;
    /// @brief Destructor.
    ~ZAPOptions();
    /// @}
private:
    class ZAPOptionsImpl; 
    std::unique_ptr<ZAPOptionsImpl> pImpl;
};
*/
}
#endif
