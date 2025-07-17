#include <zmq.hpp>
#include "us8/messaging/zeromq/authentication/zapOptions.hpp"
#include "us8/messaging/authentication/credential/keyPair.hpp"
#include "us8/messaging/authentication/credential/userNameAndPassword.hpp"

using namespace US8::Messaging::ZeroMQ::Authentication;

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
///                              ZAP Options                                 ///
///--------------------------------------------------------------------------///
IZAPOptions::~IZAPOptions() = default;
