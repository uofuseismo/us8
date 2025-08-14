#include <atomic>
#include <string_view>
#include <thread>
#include <set>
#include <string>
#ifndef NDEBUG
#include <cassert>
#endif
#include <spdlog/spdlog.h>
#include <zmq.hpp>
#include <zmq_addon.hpp>
#include "us8/broadcasts/dataPacket/subscriber.hpp"
#include "us8/broadcasts/dataPacket/subscriberOptions.hpp"
#include "us8/messageFormats/broadcasts/dataPacket.hpp"

using namespace US8::Broadcasts::DataPacket;

namespace
{
std::set<std::string> createMessageTypes()
{
    std::set<std::string> result;
    US8::MessageFormats::Broadcasts::DataPacket dataPacket;    
    result.insert(dataPacket.getMessageType());
    return result;
}
}

class Subscriber::SubscriberImpl
{
public:
    explicit SubscriberImpl(const SubscriberOptions &options) :
        mOptions(options)
    {
        // Initialize ZMQ subscriber
        try
        {
            mSubscriberSocket.set(zmq::sockopt::rcvhwm,
                                  mOptions.getHighWaterMark());
            for (const auto &messageType : mMessageTypes)
            {
                mSubscriberSocket.set(zmq::sockopt::subscribe,
                                      messageType);
            }
            auto timeOutMilliSeconds
                = static_cast<int> (mOptions.getTimeOut().count());
            if (timeOutMilliSeconds < 0)
            {
                spdlog::warn("Subscriber may wait indefinitely for message");
            }
            mSubscriberSocket.set(zmq::sockopt::rcvtimeo,
                                  timeOutMilliSeconds);
            spdlog::info("Subscriber connecting to "
                       + mOptions.getEndPoint());
            mSubscriberSocket.connect(mOptions.getEndPoint());
        }
        catch (const std::exception &e) 
        {
            auto errorMessage
                = "Failed to initialize subscriber socket because "
                + std::string {e.what()};
            throw std::runtime_error(errorMessage);
        }
        mInitialized = true;
    }
    ~SubscriberImpl()
    {   
        stop();
    }
    void start()
    {
        if (!mInitialized)
        {
             throw std::runtime_error("Subscriber not initialized");
        }
        stop();
        mKeepRunning = true;
        mSubscriberThread = std::thread(&SubscriberImpl::listen, this);
    }
    void stop() 
    {
        mKeepRunning = false;
        if (mSubscriberThread.joinable()){mSubscriberThread.join();}
    }   
    /// Listen and propagate data packets
    void listen()
    {
        auto callback = mOptions.getCallback();
        auto logInterval = mOptions.getLoggingInterval();
        auto doLogging = logInterval.count() >= 0 ? true : false;
        spdlog::debug("Thread entering listener");
        auto nowMuSeconds
           = std::chrono::time_point_cast<std::chrono::microseconds>
             (std::chrono::high_resolution_clock::now()).time_since_epoch();
        auto lastLogTime
            = std::chrono::duration_cast<std::chrono::seconds> (nowMuSeconds);
        int64_t nReceivedMessages{0};
        int64_t nNotPropagatedMessages{0};
        while (mKeepRunning)
        {
            zmq::multipart_t messagesReceived(mSubscriberSocket);
            if (messagesReceived.empty()){continue;}
            nReceivedMessages = nReceivedMessages + 1;
#ifndef NDEBUG
            assert(static_cast<int> (messagesReceived.size()) == 2);
#else
            if (static_cast<int> (messagesReceived.size()) != 2)
            {
                spdlog::warn("Only 2-part messages handled");
                nNotPropagatedMessages = nNotPropagatedMessages + 1;
                continue;
            }
#endif
            try
            {
                auto messageType = messagesReceived.at(0).to_string();
                if (!mMessageTypes.contains(messageType))
                {
                    spdlog::warn("Unhandled message type " + messageType);
                    continue;
                }
                const auto payload
                    = static_cast<char *> (messagesReceived.at(1).data());
                const auto messageSize
                    = static_cast<size_t> (messagesReceived.at(1).size());
                std::string_view messageView{payload, messageSize};
                US8::MessageFormats::Broadcasts::DataPacket
                    dataPacket{messageView}; //payload, messageSize};
                callback(std::move(dataPacket));
            }
            catch (const std::exception &e)
            {
                spdlog::warn("Failed getting from wire to queue because "
                           + std::string {e.what()});
                nNotPropagatedMessages = nNotPropagatedMessages + 1;
            }
            nowMuSeconds
                = std::chrono::time_point_cast<std::chrono::microseconds>
                 (std::chrono::high_resolution_clock::now()).time_since_epoch();
            auto nowSeconds
                = std::chrono::duration_cast<std::chrono::seconds>
                  (nowMuSeconds);
            if (doLogging && nowSeconds >= lastLogTime + logInterval)
            {
                spdlog::info("Received "
                    + std::to_string(nReceivedMessages)
                    + " messages in last "
                    + std::to_string(logInterval.count())
                    + " seconds. (Did not propagate "
                    + std::to_string(nNotPropagatedMessages) + " messages.)");
                nReceivedMessages = 0;
                nNotPropagatedMessages = 0;
                lastLogTime = nowSeconds;
            }
        }
        spdlog::debug("Thread leaving ");
    }
//private:
    SubscriberOptions mOptions;
    std::set<std::string> mMessageTypes{::createMessageTypes()};
    std::thread mSubscriberThread;
    zmq::context_t mSubscriberContext{1};
    zmq::socket_t mSubscriberSocket{mSubscriberContext, zmq::socket_type::sub};
    std::atomic<bool> mKeepRunning{true};
    bool mInitialized{false};
};

/// Constructor
Subscriber::Subscriber(const SubscriberOptions &options) :
    pImpl(std::make_unique<SubscriberImpl> (options))
{
}

/// Listen
void Subscriber::start()
{
    pImpl->start();
}

/// Stop
void Subscriber::stop()
{
    pImpl->start();
}

/// Destructor
Subscriber::~Subscriber() = default;
