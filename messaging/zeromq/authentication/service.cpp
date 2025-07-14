#include <chrono>
#include <string>
#include <array>
#include <ostream>
#include <thread>
#include <atomic>
#ifndef NDEBUG
#include <cassert>
#endif
#include <spdlog/spdlog.h>
#include <zmq.hpp>
#include <zmq_addon.hpp>
#include "us8/messaging/zeromq/authentication/service.hpp"
#include "us8/messaging/authentication/credential/userNameAndPassword.hpp"
#include "us8/messaging/authentication/exceptions.hpp"

/// Magic place where ZMQ will send authentication requests to.
#define ZAP_ENDPOINT  "inproc://zeromq.zap.01"
#define TERMINATE "TERMINATE"

using namespace US8::Messaging::ZeroMQ::Authentication;

namespace UExcept = US8::Messaging::Authentication::Exceptions;

class Service::ServiceImpl
{
public:
    explicit ServiceImpl(std::shared_ptr<zmq::context_t> context) :
        mContext(context),
        mAuthenticator{std::make_unique<Grasslands> ()}
    {
        if (mContext == nullptr)
        {
            throw std::invalid_argument("Context cannot be null");
        }

        makeEndPointName();
        spdlog::debug("Creating authentication service end point at: "
                    + mEndPoint);
        mAPIPipe
            = std::make_unique<zmq::socket_t>
              (*mContext, zmq::socket_type::pair);
        mAPIPipe->set(zmq::sockopt::linger, 1); 
        mAPIPipe->bind(mEndPoint);
    }
    ServiceImpl(std::shared_ptr<zmq::context_t> context,
                Grasslands &&grasslands) :
        mContext(context),
        mAuthenticator{std::make_unique<Grasslands> (std::move(grasslands))} 
    {
        if (mContext == nullptr)
        {
            throw std::invalid_argument("Context cannot be null");
        }   

        makeEndPointName();
        spdlog::debug("Creating authentication service end point at: "
                    + mEndPoint);
        mAPIPipe
            = std::make_unique<zmq::socket_t>
              (*mContext, zmq::socket_type::pair);
        mAPIPipe->set(zmq::sockopt::linger, 1);
        mAPIPipe->bind(mEndPoint);
    }
    ~ServiceImpl()
    {
        stop();
    }
    /// Convenience function to make endpoint name
    void makeEndPointName()
    {
        std::ostringstream address;
        address << static_cast<void const *> (this);
        mEndPoint = "inproc://" + address.str() + ".inproc";
    }
    /// Start the authenticator
    void start()
    {
        spdlog::debug("Starting the authenticator on " + mEndPoint);
        stop();
        mKeepRunning = true;
        mZAPThread = std::thread(&::Service::ServiceImpl::runZAPThread, this); 
    }
    /// What the ZAP thread does to keep busy 
    void runZAPThread()
    {
        zmq::socket_t apiPipe(*mContext, zmq::socket_type::pair);
        apiPipe.set(zmq::sockopt::linger, 1);
        try
        {
            apiPipe.connect(mEndPoint);
        }
        catch (const std::exception &e)
        {
            auto error = "Failed to connect API pipe.  ZMQ failed with "
                       + std::string(e.what());
            throw std::runtime_error(error);
        }
        mAPIStarted = true;

        // Create a ZAP socket
        spdlog::info("Binding to ZAP socket at "
                   + std::string {ZAP_ENDPOINT});
        zmq::socket_t zap(*mContext, zmq::socket_type::rep);
        zap.bind(ZAP_ENDPOINT);

        // Let API pipe know I'm ready with an empty message
        //apiPipe.send(zmq::message_t{}, zmq::send_flags::none);
        spdlog::info("Starting authenticator on endpoint " + mEndPoint);
        apiPipe.send(zmq::message_t {}, zmq::send_flags::none);
        std::array<zmq::pollitem_t, 2> pollItems =
        {
            { {apiPipe.handle(), 0, ZMQ_POLLIN, 0},
              {zap.handle(),  0, ZMQ_POLLIN, 0} }
        };
        // Wait indefinitely for another thread to release me
        constexpr std::chrono::milliseconds pollTimeOutMilliSeconds{-1};
        while (keepRunning())
        {
            zmq::poll(//pollItems, nPollItems, //
                      pollItems.data(), pollItems.size(),
                      pollTimeOutMilliSeconds);
            // API request
            if (pollItems[0].revents & ZMQ_POLLIN)
            {
                spdlog::info("API request received");
                zmq::multipart_t messagesReceived;
                auto okay = messagesReceived.recv(apiPipe);
                if (okay)
                {
                    std::string command{messagesReceived.at(0).to_string()};
                    if (command == TERMINATE)
                    {
                        spdlog::debug("Terminating service");
                        mKeepRunning = false;
                        mAPIStarted = false;
                    }
                    else
                    {
                        spdlog::warn("Unhandled command: " + command
                                   + " in ZAP poller service");
                    }
                }
                else
                {
                    spdlog::warn("Received API poll signal but no message");
                }
            }
            if (pollItems[1].revents & ZMQ_POLLIN)
            {
                spdlog::info("ZAP request received");
                zmq::multipart_t messagesReceived;
                auto okay = messagesReceived.recv(zap);
                if (okay && mKeepRunning)
                {
#ifndef NDEUBG
                    assert(messagesReceived.size() >= 6);
#endif
                    // Order is defined in:
                    // https://rfc.zeromq.org/spec/27/
                    auto domain    = messagesReceived.at(2).to_string(); // e.g., global
                    auto ipAddress = messagesReceived.at(3).to_string();
                    auto identity  = messagesReceived.at(4).to_string(); // Originating socket ID
                    auto mechanism = messagesReceived.at(5).to_string();

                    int statusCode{500};
                    std::string statusText{"Server error"}; 
                    if (mAuthenticator)
                    {
                        try
                        {
                            mAuthenticator->isWhiteListed(ipAddress);
                            mAuthenticator->isBlackListed(ipAddress);
                            if (mechanism == "PLAIN")
                            {
                                auto user = messagesReceived.at(5).to_string();
                                auto password
                                    = messagesReceived.at(6).to_string();
                                if (user.empty())
                                {
                                    throw UExcept::BadRequest(
                                       "User name is empty");
                                }
                                US8::Messaging::Authentication::Credential
                                ::UserNameAndPassword plainText{user, password};
                            } 
                            else if (mechanism == "CURVE")
                            {
                                if (messagesReceived.at(6).size() != 32)
                                {
                                    throw UExcept::BadRequest(
                                       "Key must be length 32");
                                }
                                auto keyPtr = reinterpret_cast<const uint8_t *>
                                              (messagesReceived.at(6).data());

                                std::array<uint8_t, 32> publicKey{};

                            }
                            else
                            {
                                if (mechanism != "NULL")
                                {
                                    throw UExcept::InternalServerError(
                                        "Undefined ZAP mechanism");
                                }
                            }
                            // Made it this far without throwing - you're okay
                            spdlog::info("Allowing connection from " + ipAddress
                                       + " on domain " + domain);
                            statusCode = 200;
                            statusText = "OK";
                        }
                        catch (const UExcept::BadRequest &e)
                        {
                            spdlog::debug("Bad request received from "
                                        + ipAddress);
                            statusCode = 400;
                            statusText = e.what();
                        }
                        catch (const UExcept::Unauthorized &e)
                        {
                            spdlog::info(ipAddress + " is not authorized");
                            statusCode = 401;
                            statusText = "Unauthorized";
                        }
                        catch (const UExcept::Forbidden &e)
                        {
                            spdlog::info(ipAddress + " is forbidden");
                            statusCode = 403;
                            statusText = "Forbidden";
                        }
                        catch (const UExcept::InternalServerError &e)
                        {
                            spdlog::warn("Internal error "
                                       + std::string {e.what()});
                            statusCode = 500;
                            statusText = "Internal server error"; 
                        }
                        catch (const std::exception &e)
                        { 
                            spdlog::warn("Unhandled exception "
                                       + std::string {e.what()});
                            spdlog::warn(e.what());
                            statusCode = 500;
                            statusText = "Internal server error";
                        }
                    }
                    else
                    {
                        spdlog::warn("No authenticator set - forbidding access to " + ipAddress);
                    }
                    // Format result.  The order is defined in:
                    // https://rfc.zeromq.org/spec/27/
                    zmq::multipart_t reply;
                    reply.addstr(messagesReceived.at(0).to_string()); // Version
                    reply.addstr(messagesReceived.at(1).to_string()); // Sequence Number 
                    reply.addstr(std::to_string(statusCode));
                    reply.addstr(statusText);
                    reply.addstr(mAuthenticatorIdentifier);
                    reply.addstr(""); // Always end with this
                    reply.send(zap);
                }
                else
                {
                    spdlog::warn("Received ZAP poll signal but no message");
                }
            }
        }
        spdlog::info("Terminating authenticator on endpoint " + mEndPoint);
        apiPipe.close();
        zap.close();
    }
    [[nodiscard]] bool keepRunning() const noexcept
    {
        return mKeepRunning;
    }
    void stop()
    {
        if (mAPIStarted)
        {
            spdlog::debug("Sending TERMINATE message");
            mAPIPipe->send(zmq::str_buffer(TERMINATE), zmq::send_flags::none);
            mAPIStarted = false;
        }
        mKeepRunning = false;
        if (mZAPThread.joinable()){mZAPThread.join();}
    }
//private:
    std::thread mZAPThread;
    std::string mAuthenticatorIdentifier{"US8ZMAAuth"};
    std::unique_ptr<US8::Messaging::Authentication::IAuthenticator>
        mAuthenticator{nullptr};
    std::shared_ptr<zmq::context_t> mContext{nullptr};
    std::unique_ptr<zmq::socket_t> mAPIPipe{nullptr};
    std::string mEndPoint;
    std::atomic<bool> mAPIStarted{false};
    std::atomic<bool> mKeepRunning{true};
};

/// Constructor
Service::Service(std::shared_ptr<zmq::context_t> context,
                 Grasslands &&grasslands) :
    pImpl(std::make_unique<ServiceImpl> (context, std::move(grasslands)))
{
}

/// Start the service
void Service::start()
{
    pImpl->start();
}

/// Stop the service
void Service::stop()
{
    pImpl->stop();
}

/// Destructor
Service::~Service() = default;
