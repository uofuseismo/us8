#include <string>
#include <algorithm>
#include "us8/broadcasts/dataPacket/subscriberOptions.hpp"
#include "us8/messageFormats/broadcasts/dataPacket.hpp"

using namespace US8::Broadcasts::DataPacket;

class SubscriberOptions::SubscriberOptionsImpl
{
public:
    std::function<void (US8::MessageFormats::Broadcasts::DataPacket &&)>
         mCallback;
    std::string mEndPoint;
    std::chrono::seconds mLoggingInterval{3600};
    std::chrono::milliseconds mReceiveTimeOut{10};
    int mReceiveHighWaterMark{4096};
    bool mHaveCallback{false};
};

/// Constructor
SubscriberOptions::SubscriberOptions(
    const std::string &endPointIn,
    const std::function<void (US8::MessageFormats::Broadcasts::DataPacket &&)> &callback) :
    pImpl(std::make_unique<SubscriberOptionsImpl> ())
{
    auto endPoint = endPointIn;
    endPoint.erase(std::remove_if(endPoint.begin(), endPoint.end(), ::isspace),
                   endPoint.end());
    if (endPoint.empty())
    {   
        throw std::invalid_argument("End point is empty");
    }
    if (!endPoint.starts_with("tcp://") &&
        !endPoint.starts_with("udp://") &&
        !endPoint.starts_with("inproc://"))
    {
        throw std::invalid_argument(
           "End point must start with tcp:// or udp:// or inproc://");
    }   
    pImpl->mEndPoint = endPoint;
    pImpl->mCallback = callback;
    pImpl->mHaveCallback = true;
}

/// Copy constructor
SubscriberOptions::SubscriberOptions(const SubscriberOptions &options)
{
    *this = options;
}

/// Move constructor
SubscriberOptions::SubscriberOptions(SubscriberOptions &&options) noexcept
{
    *this = std::move(options);
}

/// Copy assignment
SubscriberOptions& 
SubscriberOptions::operator=(const SubscriberOptions &options)
{
    if (&options == this){return *this;}
    pImpl = std::make_unique<SubscriberOptionsImpl> (*options.pImpl);
    return *this;
}

/// Move assignment
SubscriberOptions& 
SubscriberOptions::operator=(SubscriberOptions &&options) noexcept
{
    if (&options == this){return *this;}
    pImpl = std::move(options.pImpl);
    return *this;
}

/// Destructor
SubscriberOptions::~SubscriberOptions() = default;

/// End point
/*
void SubscriberOptions::setEndPoint(const std::string &endPointIn)
{
    auto endPoint = endPointIn;
    endPoint.erase(std::remove_if(endPoint.begin(), endPoint.end(), ::isspace),
                   endPoint.end());
    if (endPoint.empty())
    {
        throw std::invalid_argument("End point is empty");
    }
    if (!endPoint.starts_with("tcp://") &&
        !endPoint.starts_with("udp://") &&
        !endPoint.starts_with("inproc://"))
    {
        throw std::invalid_argument(
           "End point must start with tcp:// or udp:// or inproc://");
    }
    pImpl->mEndPoint = endPoint;
}
*/

std::string SubscriberOptions::getEndPoint() const
{
    if (pImpl->mEndPoint.empty())
    {
        throw std::invalid_argument("End point not set");
    }
    return pImpl->mEndPoint;
}

/// Callback
/*
void SubscriberOptions::setCallback(
    const std::function<void (US8::MessageFormats::Broadcasts::DataPacket &&)> &callback) noexcept
{
    pImpl->mCallback = callback;
    pImpl->mHaveCallback = true;
}
*/

std::function<void (US8::MessageFormats::Broadcasts::DataPacket &&)>
    SubscriberOptions::getCallback() const
{
    if (!pImpl->mHaveCallback)
    {
        throw std::runtime_error("Callback not set");
    }
    return pImpl->mCallback;
}

/// Timeout
void SubscriberOptions::setTimeOut(
    const std::chrono::milliseconds &timeOut) noexcept
{
    pImpl->mReceiveTimeOut = timeOut;
    if (timeOut.count() < 0)
    {
        pImpl->mReceiveTimeOut = std::chrono::milliseconds {-1};
    }
}

std::chrono::milliseconds SubscriberOptions::getTimeOut() const noexcept
{
    return pImpl->mReceiveTimeOut;
}

/// High water mark
void SubscriberOptions::setHighWaterMark(const int highWaterMark)
{
    if (highWaterMark < 0)
    {
        throw std::invalid_argument("High water mark must be non-negative");
    }
    pImpl->mReceiveHighWaterMark = highWaterMark;
}

int SubscriberOptions::getHighWaterMark() const noexcept
{
    return pImpl->mReceiveHighWaterMark;
}

/// Logging interval
void SubscriberOptions::setLoggingInterval(
    const std::chrono::seconds &loggingInterval) noexcept
{
    pImpl->mLoggingInterval = loggingInterval;
}

std::chrono::seconds SubscriberOptions::getLoggingInterval() const noexcept
{
    return pImpl->mLoggingInterval;
}
