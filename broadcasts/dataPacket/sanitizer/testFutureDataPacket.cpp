#include <iostream>
#include <string>
#include <chrono>
#include <mutex>
#include <set>
#include <spdlog/spdlog.h>
//#include "us8/broadcasts/dataPacket/sanitizer/testFutureDataPacket.hpp"
#include "testFutureDataPacket.hpp"
#include "us8/messageFormats/broadcasts/dataPacket.hpp"
#include "toName.hpp"

using namespace US8::Broadcasts::DataPacket::Sanitizer;

class TestFutureDataPacket::TestFutureDataPacketImpl
{
public:
    TestFutureDataPacketImpl(const TestFutureDataPacketImpl &impl)
    {
        *this = impl;
    }
    TestFutureDataPacketImpl(const std::chrono::microseconds &maxFutureTime,
                             const std::chrono::seconds &logBadDataInterval) :
        mMaxFutureTime(maxFutureTime),
        mLogBadDataInterval(logBadDataInterval)
    {
        // This might be okay if you really want to account for telemetry
        // lags.  But that's a dangerous game so I'll let the user know.
        if (mMaxFutureTime.count() < 0)
        {
            spdlog::warn("Max future time is negative");
        }
        if (mLogBadDataInterval.count() >= 0)
        {
            mLogBadData = true;
        }
        else
        {
            mLogBadData = false;
        }
    }
    /// Logs the bad events
    void logBadData(const bool allow,
                    const US8::MessageFormats::Broadcasts::DataPacket &packet,
                    const std::chrono::microseconds &nowMuSec)
    {
        if (!mLogBadData){return;}
        std::string name;
        try
        {
            if (!allow){name = ::toName(packet);}
        }
        catch (...)
        {
            spdlog::warn("Could not extract name of packet");
        }
        auto nowSeconds
            = std::chrono::duration_cast<std::chrono::seconds> (nowMuSec);
        {
        std::lock_guard<std::mutex> lockGuard(mMutex); 
        try
        {
            if (!name.empty() && !mFutureChannels.contains(name))
            {
                mFutureChannels.insert(name);
            }
        }
        catch (...)
        {
            spdlog::warn("Failed to add " + name + " to set");
        }
        if (nowSeconds >= mLastLogTime + mLogBadDataInterval)
        {
            if (!mFutureChannels.empty())
            {
                std::string message{"Future data detected for: "};
                for (const auto &channel : mFutureChannels)
                {
                    message = message + " " + channel;
                }
                spdlog::info(message);
                mFutureChannels.clear();
                mLastLogTime = nowSeconds;
            }
        }
        }
    }
    TestFutureDataPacketImpl& operator=(const TestFutureDataPacketImpl &impl)
    {
        if (&impl == this){return *this;}
        {
        std::lock_guard<std::mutex> lockGuard(impl.mMutex);
        mFutureChannels = impl.mFutureChannels;
        mLastLogTime = impl.mLastLogTime; 
        }
        mMaxFutureTime = impl.mMaxFutureTime;
        mLogBadDataInterval = impl.mLogBadDataInterval;
        mLogBadData = impl.mLogBadData;
        return *this;
    }
//private:
    mutable std::mutex mMutex;
    std::set<std::string> mFutureChannels;
    std::chrono::microseconds mMaxFutureTime{0};
    std::chrono::seconds mLastLogTime{0};
    std::chrono::seconds mLogBadDataInterval{3600};
    bool mLogBadData{true};
};

/// Constructor
TestFutureDataPacket::TestFutureDataPacket() :
    pImpl(std::make_unique<TestFutureDataPacketImpl>
          (std::chrono::microseconds {0},
           std::chrono::seconds {3600}))
{
}

/// Constructor with options
TestFutureDataPacket::TestFutureDataPacket(
    const std::chrono::microseconds &maxFutureTime,
    const std::chrono::seconds &logBadDataInterval) :
    pImpl(std::make_unique<TestFutureDataPacketImpl> (maxFutureTime,
                                                  logBadDataInterval))
{
}

/// Copy constructor
TestFutureDataPacket::TestFutureDataPacket(
    const TestFutureDataPacket &testFutureDataPacket)
{
    *this = testFutureDataPacket;
}

/// Move constructor
TestFutureDataPacket::TestFutureDataPacket(
    TestFutureDataPacket &&testFutureDataPacket) noexcept
{
    *this = std::move(testFutureDataPacket);
}

/// Copy assignment
TestFutureDataPacket& 
TestFutureDataPacket::operator=(const TestFutureDataPacket &testFutureDataPacket)
{
    if (&testFutureDataPacket == this){return *this;}
    pImpl = std::make_unique<TestFutureDataPacketImpl> (*testFutureDataPacket.pImpl);
    return *this;
}

/// Move assignment
TestFutureDataPacket&
TestFutureDataPacket::operator=(TestFutureDataPacket &&testFutureDataPacket) noexcept
{
    if (&testFutureDataPacket == this){return *this;}
    pImpl = std::move(testFutureDataPacket.pImpl);
    return *this;
}

/// Destructor
TestFutureDataPacket::~TestFutureDataPacket() = default;

/// Does the work
bool TestFutureDataPacket::allow(
    const US8::MessageFormats::Broadcasts::DataPacket &packet) const
{
    auto packetEndTime = packet.getEndTime(); // Throws
    // Computing the current time after the scraping the ring is
    // conservative.  Basically, when the max future time is zero,
    // this allows for a zero-latency, 1 sample packet, to be
    // successfully passed through.
    auto now = std::chrono::high_resolution_clock::now();
    auto nowMuSeconds
        = std::chrono::time_point_cast<std::chrono::microseconds>
          (now).time_since_epoch();
    auto latestTime  = nowMuSeconds + pImpl->mMaxFutureTime;
    // Packet contains data after max allowable time?
    bool allow = (packetEndTime <= latestTime) ? true : false;
    // (Safely) handle logging
    try
    {
        pImpl->logBadData(allow, packet, nowMuSeconds);
    }
    catch (const std::exception &e)
    {
        spdlog::warn("Error detect in logBadData: "
                   + std::string {e.what()});
    }
    return allow;
}
