#include <string>
#include <algorithm>
#include "us8/broadcasts/dataPacket/publisherOptions.hpp"

using namespace US8::Broadcasts::DataPacket;

class PublisherOptions::PublisherOptionsImpl
{
public:
    std::string mEndPoint;
    std::chrono::milliseconds mSendTimeOut{10};
    int mSendHighWaterMark{4096};
    bool mHaveCallback{false};
};

/// Constructor
PublisherOptions::PublisherOptions(const std::string &endPointIn) :
    pImpl(std::make_unique<PublisherOptionsImpl> ())
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

/// Copy constructor
PublisherOptions::PublisherOptions(const PublisherOptions &options)
{
    *this = options;
}

/// Move constructor
PublisherOptions::PublisherOptions(PublisherOptions &&options) noexcept
{
    *this = std::move(options);
}

/// Copy assignment
PublisherOptions& 
PublisherOptions::operator=(const PublisherOptions &options)
{
    if (&options == this){return *this;}
    pImpl = std::make_unique<PublisherOptionsImpl> (*options.pImpl);
    return *this;
}

/// Move assignment
PublisherOptions& 
PublisherOptions::operator=(PublisherOptions &&options) noexcept
{
    if (&options == this){return *this;}
    pImpl = std::move(options.pImpl);
    return *this;
}

/// Destructor
PublisherOptions::~PublisherOptions() = default;

std::string PublisherOptions::getEndPoint() const
{
    if (pImpl->mEndPoint.empty())
    {
        throw std::invalid_argument("End point not set");
    }
    return pImpl->mEndPoint;
}

/// Timeout
void PublisherOptions::setTimeOut(
    const std::chrono::milliseconds &timeOut) noexcept
{
    pImpl->mSendTimeOut = timeOut;
    if (timeOut.count() < 0)
    {
        pImpl->mSendTimeOut = std::chrono::milliseconds {-1};
    }
}

std::chrono::milliseconds PublisherOptions::getTimeOut() const noexcept
{
    return pImpl->mSendTimeOut;
}

/// High water mark
void PublisherOptions::setHighWaterMark(const int highWaterMark)
{
    if (highWaterMark < 0)
    {
        throw std::invalid_argument("High water mark must be non-negative");
    }
    pImpl->mSendHighWaterMark = highWaterMark;
}

int PublisherOptions::getHighWaterMark() const noexcept
{
    return pImpl->mSendHighWaterMark;
}

/// Logging interval
/*
void PublisherOptions::setLoggingInterval(
    const std::chrono::seconds &loggingInterval) noexcept
{
    pImpl->mLoggingInterval = loggingInterval;
}

std::chrono::seconds PublisherOptions::getLoggingInterval() const noexcept
{
    return pImpl->mLoggingInterval;
}
*/
