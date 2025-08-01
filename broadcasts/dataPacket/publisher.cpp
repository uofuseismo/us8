#include <string>
#ifndef NDEBUG
#include <cassert>
#endif
#include <spdlog/spdlog.h>
#include <zmq.hpp>
#include <zmq_addon.hpp>
#include "us8/broadcasts/dataPacket/publisher.hpp"
#include "us8/broadcasts/dataPacket/publisherOptions.hpp"
#include "us8/messageFormats/broadcasts/dataPacket.hpp"

using namespace US8::Broadcasts::DataPacket;

class Publisher::PublisherImpl
{   
public:
    explicit PublisherImpl(const PublisherOptions &options) :
        mOptions(options)
    {   
        // Initialize ZMQ subscriber
        try
        {
            mPublisherSocket.set(zmq::sockopt::sndhwm,
                                  mOptions.getHighWaterMark());
            auto timeOutMilliSeconds
                = static_cast<int> (mOptions.getTimeOut().count());
            if (timeOutMilliSeconds < 0)
            {
                spdlog::warn("Publisher may wait indefinitely to send message");
            }
            mPublisherSocket.set(zmq::sockopt::sndtimeo,
                                  timeOutMilliSeconds);
            spdlog::info("Publisher connecting to "
                       + mOptions.getEndPoint());
            mPublisherSocket.connect(mOptions.getEndPoint());
        }
        catch (const std::exception &e) 
        {
            auto errorMessage
                = "Failed to initialize publisher socket because "
                + std::string {e.what()};
            throw std::runtime_error(errorMessage);
        }
        mInitialized = true;
    }
    /// Sends a message
    void send(const US8::MessageFormats::Broadcasts::DataPacket &dataPacket)
    {
        auto messageType = dataPacket.getMessageType(); // Throws
#ifndef NDEBUG
        if (messageType.empty())
        {
            throw std::runtime_error("Message type for data packet is empty");
        }
#endif
        auto messagePayload = dataPacket.serialize();
        std::array<zmq::const_buffer, 2> messages{
            zmq::const_buffer {messageType.data(),
                               messageType.size()},
            zmq::const_buffer {messagePayload.data(),
                               messagePayload.size()}
        };
        auto nPartsSent
            = zmq::send_multipart(mPublisherSocket, messages);
        if (nPartsSent != 2)
        {
            throw std::runtime_error(
                "Failed to send two-part message");
        }
    } 
//public:
    PublisherOptions mOptions;
    zmq::context_t mPublisherContext{1};
    zmq::socket_t mPublisherSocket{mPublisherContext, zmq::socket_type::pub};
    bool mInitialized{false};
};

/// Constructor
Publisher::Publisher(const PublisherOptions &options) :
    pImpl(std::make_unique<PublisherImpl> (options))
{
}

/// Send
void Publisher::send(
    const US8::MessageFormats::Broadcasts::DataPacket &dataPacket)
{
    if (!pImpl->mInitialized)
    {
        throw std::invalid_argument("Publisher not initialized");
    }
    pImpl->send(dataPacket);
}

void Publisher::operator()(
    const US8::MessageFormats::Broadcasts::DataPacket &dataPacket)
{
    send(dataPacket);
}

/// Destructor
Publisher::~Publisher() = default;
