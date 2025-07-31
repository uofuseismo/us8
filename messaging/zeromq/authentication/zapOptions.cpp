#include <zmq.hpp>
#include <vector>
#include "us8/messaging/zeromq/authentication/zapOptions.hpp"
#include "us8/messaging/authentication/credential/keyPair.hpp"
#include "us8/messaging/authentication/credential/userNameAndPassword.hpp"

using namespace US8::Messaging::ZeroMQ::Authentication;

namespace
{
[[nodiscard]] std::string keyToString(const std::vector<uint8_t> &key)
{
    std::string result;
    result.resize(key.size());
    std::copy(key.begin(), key.end(), result.data());
    return result;
}
}

///--------------------------------------------------------------------------///
///                             Grasslands Client                            ///
///--------------------------------------------------------------------------///
class GrasslandsClient::GrasslandsClientImpl
{
public:

};

/// Constructor
GrasslandsClient::GrasslandsClient() :
    pImpl(std::make_unique<GrasslandsClientImpl> ())
{
}

/// Copy constructor
GrasslandsClient::GrasslandsClient(const GrasslandsClient &client)
{
    *this = client;
}

/// Move constructor
GrasslandsClient::GrasslandsClient(GrasslandsClient &&client) noexcept
{
    *this = std::move(client);
}

/// Copy assignment
GrasslandsClient& GrasslandsClient::operator=(const GrasslandsClient &client)
{
    if (&client == this){return *this;}
    pImpl = std::make_unique<GrasslandsClientImpl> (*client.pImpl);
    return *this;
}

/// Move assignment
GrasslandsClient& 
GrasslandsClient::operator=(GrasslandsClient &&client) noexcept
{
    if (&client == this){return *this;}
    pImpl = std::move(client.pImpl);
    return *this;
}

/// Destructor
GrasslandsClient::~GrasslandsClient() = default;

/// Protocol
Protocol GrasslandsClient::getProtocol() const noexcept
{
    return Protocol::Grasslands;
}

/// Auth server?
bool GrasslandsClient::isAuthenticationServer() const noexcept
{
    return false;
}

/// ZAP options
void GrasslandsClient::setSocketOptions(zmq::socket_t *options) const
{
    if (options == nullptr){throw std::runtime_error("Socket pointer is NULL");}
}

///--------------------------------------------------------------------------///
///                             Grasslands Server                            ///
///--------------------------------------------------------------------------///
class GrasslandsServer::GrasslandsServerImpl
{
public:
    std::string mDomain{"global"};
};

/// Constructor
GrasslandsServer::GrasslandsServer() :
    pImpl(std::make_unique<GrasslandsServerImpl> ()) 
{
}

/// Copy constructor
GrasslandsServer::GrasslandsServer(const GrasslandsServer &server)
{
    *this = server;
}

/// Move constructor
GrasslandsServer::GrasslandsServer(GrasslandsServer &&server) noexcept
{
    *this = std::move(server);
}

/// Copy assignment
GrasslandsServer& GrasslandsServer::operator=(const GrasslandsServer &server)
{
    if (&server == this){return *this;}
    pImpl = std::make_unique<GrasslandsServerImpl> (*server.pImpl);
    return *this;
}

/// Move assignment
GrasslandsServer&
GrasslandsServer::operator=(GrasslandsServer &&server) noexcept
{
    if (&server == this){return *this;}
    pImpl = std::move(server.pImpl);
    return *this;
}

/// Destructor
GrasslandsServer::~GrasslandsServer() = default;

/// Protocol
Protocol GrasslandsServer::getProtocol() const noexcept
{
    return Protocol::Grasslands;
}

/// Auth server?
bool GrasslandsServer::isAuthenticationServer() const noexcept
{
    return true;
}

/// Domain
std::string GrasslandsServer::getDomain() const noexcept
{
    return pImpl->mDomain;
}

/// ZAP options
void GrasslandsServer::setSocketOptions(zmq::socket_t *options) const
{
    if (options == nullptr){throw std::runtime_error("Socket pointer is NULL");}
    options->set(zmq::sockopt::zap_domain, getDomain());
}

///--------------------------------------------------------------------------///
///                               Strawhouse                                 ///
///--------------------------------------------------------------------------///
class StrawhouseServer::StrawhouseServerImpl
{
public:
    std::string mDomain{"global"};
};

/// Constructor
StrawhouseServer::StrawhouseServer() :
    pImpl(std::make_unique<StrawhouseServerImpl> ()) 
{
}

/// Copy constructor
StrawhouseServer::StrawhouseServer(const StrawhouseServer &server)
{
    *this = server;
}

/// Move constructor
StrawhouseServer::StrawhouseServer(StrawhouseServer &&server) noexcept
{
    *this = std::move(server);
}

/// Copy assignment
StrawhouseServer& StrawhouseServer::operator=(const StrawhouseServer &server)
{
    if (&server == this){return *this;}
    pImpl = std::make_unique<StrawhouseServerImpl> (*server.pImpl);
    return *this;
}

/// Move assignment
StrawhouseServer&
StrawhouseServer::operator=(StrawhouseServer &&server) noexcept
{
    if (&server == this){return *this;}
    pImpl = std::move(server.pImpl);
    return *this;
}

/// Destructor
StrawhouseServer::~StrawhouseServer() = default;

/// Protocol
Protocol StrawhouseServer::getProtocol() const noexcept
{
    return Protocol::Strawhouse;
}
    
/// Auth server?
bool StrawhouseServer::isAuthenticationServer() const noexcept
{
    return true;
}
    
/// Domain
std::string StrawhouseServer::getDomain() const noexcept
{
    return pImpl->mDomain;
}

/// ZAP options
void StrawhouseServer::setSocketOptions(zmq::socket_t *options) const
{   
    if (options == nullptr){throw std::runtime_error("Socket pointer is NULL");}
    options->set(zmq::sockopt::zap_domain, getDomain());
}

///--------------------------------------------------------------------------///
///                        Stonehouse client                                 ///
///--------------------------------------------------------------------------///
class StonehouseClient::StonehouseClientImpl
{
public:
    std::unique_ptr<US8::Messaging::Authentication::Credential::KeyPair>
        mClientKeyPair{nullptr};
    std::unique_ptr<US8::Messaging::Authentication::Credential::KeyPair>
        mServerPublicKey{nullptr};
    std::string mDomain{"global"};
};

/// Constructor
StonehouseClient::StonehouseClient(
    const US8::Messaging::Authentication::Credential::KeyPair &clientKeyPair,
    const US8::Messaging::Authentication::Credential::KeyPair &serverPublicKey):
    pImpl(std::make_unique<StonehouseClientImpl> ())
{
    constexpr size_t zeroMQKeySize{32};
    if (clientKeyPair.getPublicKey().size() != zeroMQKeySize)
    {
        throw std::invalid_argument(
            "Client public key must have length "
           + std::to_string(zeroMQKeySize));
    }
    if (!clientKeyPair.getPrivateKey())
    {
        throw std::invalid_argument("Client's private key not set");
    }
    if (clientKeyPair.getPrivateKey()->size() != zeroMQKeySize)
    {
        throw std::invalid_argument(
            "Client private key must have length "
           + std::to_string(zeroMQKeySize));
    }
    if (serverPublicKey.getPublicKey().size() != zeroMQKeySize)
    {   
        throw std::invalid_argument(
            "Server public key must have length "
           + std::to_string(zeroMQKeySize));
    }
    pImpl->mClientKeyPair
        = std::make_unique<US8::Messaging::Authentication::Credential::KeyPair>
          (clientKeyPair);
    pImpl->mServerPublicKey
        = std::make_unique<US8::Messaging::Authentication::Credential::KeyPair>
          (serverPublicKey);
}

/// Copy constructor
StonehouseClient::StonehouseClient(const StonehouseClient &client)
{
    *this = client;
}

/// Move constructor
StonehouseClient::StonehouseClient(StonehouseClient &&client) noexcept
{
    *this = std::move(client);
}

/// Destructor
StonehouseClient::~StonehouseClient() = default;

/// Copy assignment
StonehouseClient& 
StonehouseClient::operator=(const StonehouseClient &client)
{
    if (&client == this){return *this;}
    pImpl = std::make_unique<StonehouseClientImpl> ();
    if (client.pImpl->mClientKeyPair)
    {
        pImpl->mClientKeyPair
            = std::make_unique<US8::Messaging::Authentication::Credential::KeyPair>
              (*client.pImpl->mClientKeyPair);
    }
    if (client.pImpl->mServerPublicKey)
    {
        pImpl->mServerPublicKey
            = std::make_unique<US8::Messaging::Authentication::Credential::KeyPair>
              (*client.pImpl->mServerPublicKey);
    }
    pImpl->mDomain = client.pImpl->mDomain;
    return *this;
}

/// Move assignment
StonehouseClient& 
StonehouseClient::operator=(StonehouseClient &&client) noexcept
{
    if (&client == this){return *this;}
    pImpl = std::move(client.pImpl);
    return *this;
}

/// Domain
std::string StonehouseClient::getDomain() const noexcept
{
    return pImpl->mDomain;
}

/// Protocol
Protocol StonehouseClient::getProtocol() const noexcept
{
    return Protocol::Stonehouse;
}
        
/// Auth server?
bool StonehouseClient::isAuthenticationServer() const noexcept
{
    return false;
}
  
/// Socket options
void StonehouseClient::setSocketOptions(zmq::socket_t *options) const
{
    if (options == nullptr){throw std::runtime_error("Socket pointer is NULL");}
    if (pImpl->mServerPublicKey == nullptr)
    {
        throw std::runtime_error("Server public key not set");
    }
    if (pImpl->mClientKeyPair == nullptr)
    {
        throw std::runtime_error("Client keypair not set");
    }

    auto serverPublicKey
        = ::keyToString(pImpl->mServerPublicKey->getPublicKey());
    auto clientPublicKey
        = ::keyToString(pImpl->mClientKeyPair->getPublicKey());
    auto clientPrivateKey
        = ::keyToString(*pImpl->mClientKeyPair->getPrivateKey());

    options->set(zmq::sockopt::zap_domain, getDomain());
    options->set(zmq::sockopt::curve_server,
                 isAuthenticationServer() ? 1 : 0); 
    options->set(zmq::sockopt::curve_serverkey,
                 serverPublicKey.data());
    options->set(zmq::sockopt::curve_publickey,
                 clientPublicKey.data());
    options->set(zmq::sockopt::curve_secretkey,
                 clientPrivateKey.data());
}

///--------------------------------------------------------------------------///
///                        Stonehouse server                                 ///
///--------------------------------------------------------------------------///
class StonehouseServer::StonehouseServerImpl
{
public:
    std::unique_ptr<US8::Messaging::Authentication::Credential::KeyPair>
        mServerKeyPair{nullptr};
    std::string mDomain{"global"};
};

/// Constructor
StonehouseServer::StonehouseServer(
    const US8::Messaging::Authentication::Credential::KeyPair &serverKeyPair):
    pImpl(std::make_unique<StonehouseServerImpl> ())
{
    constexpr size_t zeroMQKeySize{32};
    if (serverKeyPair.getPublicKey().size() != zeroMQKeySize)
    {
        throw std::invalid_argument(
            "Server public key must have length "
           + std::to_string(zeroMQKeySize));
    }
    if (!serverKeyPair.getPrivateKey())
    {
        throw std::invalid_argument("Server's private key not set");
    }
    if (serverKeyPair.getPrivateKey()->size() != zeroMQKeySize)
    {
        throw std::invalid_argument(
            "Server private key must have length "
           + std::to_string(zeroMQKeySize));
    }
    pImpl->mServerKeyPair
        = std::make_unique<US8::Messaging::Authentication::Credential::KeyPair>
          (serverKeyPair);
}

/// Copy constructor
StonehouseServer::StonehouseServer(const StonehouseServer &server)
{
    *this = server;
}

/// Move constructor
StonehouseServer::StonehouseServer(StonehouseServer &&server) noexcept
{
    *this = std::move(server);
}

/// Destructor   
StonehouseServer::~StonehouseServer() = default;

/// Copy assignment
StonehouseServer&
StonehouseServer::operator=(const StonehouseServer &server)
{
    if (&server == this){return *this;}
    pImpl = std::make_unique<StonehouseServerImpl> ();
    if (server.pImpl->mServerKeyPair)
    {   
        pImpl->mServerKeyPair
            = std::make_unique<US8::Messaging::Authentication::Credential::KeyPair>
              (*server.pImpl->mServerKeyPair);
    }
    pImpl->mDomain = server.pImpl->mDomain;
    return *this;
}
    
/// Move assignment
StonehouseServer& 
StonehouseServer::operator=(StonehouseServer &&server) noexcept
{   
    if (&server == this){return *this;}
    pImpl = std::move(server.pImpl);
    return *this;
}

/// Domain
std::string StonehouseServer::getDomain() const noexcept
{
    return pImpl->mDomain;
}

/// Protocol
Protocol StonehouseServer::getProtocol() const noexcept
{
    return Protocol::Stonehouse;
}
        
/// Auth server?
bool StonehouseServer::isAuthenticationServer() const noexcept
{
    return true;
}

void StonehouseServer::setSocketOptions(zmq::socket_t *options) const
{
    auto serverPublicKey
        = ::keyToString(pImpl->mServerKeyPair->getPublicKey());
    auto serverPrivateKey 
        = ::keyToString(*pImpl->mServerKeyPair->getPrivateKey());

    options->set(zmq::sockopt::zap_domain, getDomain());
    options->set(zmq::sockopt::curve_server,
                 isAuthenticationServer() ? 1 : 0);
    options->set(zmq::sockopt::curve_publickey, serverPublicKey.data());
    options->set(zmq::sockopt::curve_secretkey, serverPrivateKey.data());
}


///--------------------------------------------------------------------------///
///                              ZAP Options                                 ///
///--------------------------------------------------------------------------///
IZAPOptions::~IZAPOptions() = default;
