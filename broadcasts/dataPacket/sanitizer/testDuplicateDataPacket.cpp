#include <iostream>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <map>
#include <set>
#include <string>
#ifndef NDEBUG
#include <cassert>
#endif
#include <boost/circular_buffer.hpp>
#include <spdlog/spdlog.h>
#include "testDuplicateDataPacket.hpp"
#include "us8/messageFormats/broadcasts/dataPacket.hpp"
#include "toName.hpp"

using namespace US8::Broadcasts::DataPacket::Sanitizer;

namespace
{

struct DataPacketHeader
{
public:
    DataPacketHeader() = default;
    explicit DataPacketHeader(
        const US8::MessageFormats::Broadcasts::DataPacket &packet)
    {
        name = ::toName(packet);
#ifndef NDEBUG
        assert(!name.empty());
#endif
        // Start and end time
        startTime = packet.getStartTime();
        endTime = packet.getEndTime(); // Throws
        // Sampling rate (approximate)
        samplingRate
            = static_cast<int> (std::round(packet.getSamplingRate()));
        // Number of samples
        nSamples = packet.getNumberOfSamples();
        if (nSamples <= 0)
        {
            throw std::invalid_argument("No samples in packet");
        }
    } 
    bool operator<(const ::DataPacketHeader &rhs) const
    {
        return startTime < rhs.startTime;
    } 
    bool operator>(const ::DataPacketHeader &rhs) const
    {
        return startTime > rhs.startTime;
    }
    bool operator==(const ::DataPacketHeader &rhs) const
    {
        if (rhs.name != name){return false;}
        if (rhs.samplingRate - samplingRate != 0)
        {
            throw std::runtime_error("Inconsistent sampling rates for: "
                                   + name);
            //return false;
        }
        if (rhs.nSamples != nSamples){return false;}
        auto dStartTime = std::abs(rhs.startTime.count() - startTime.count());
        if (samplingRate < 105)
        {
            return (dStartTime < std::chrono::microseconds {15000}.count());
        }
        else if (samplingRate < 255)
        {
            return (dStartTime < std::chrono::microseconds {4500}.count());
        }
        else if (samplingRate < 505)
        {
            return (dStartTime < std::chrono::microseconds {2500}.count());
        }
        else if (samplingRate < 1005)
        {
            return (dStartTime < std::chrono::microseconds {1500}.count());
        }
        throw std::runtime_error(
            "Could not classify sampling rate: " + std::to_string(samplingRate)
          + " for " + name);
        //return false;
    } 
    std::string name; // Packet name NETWORK.STATION.CHANNEL.LOCATION
    std::chrono::microseconds startTime{0}; // UTC time of first sample
    std::chrono::microseconds endTime{0}; // UTC time of last sample
    // Typically `observed' sampling rates wobble around a nominal sampling rate
    int samplingRate{100};
    int nSamples{0}; // Number of samples in packet
};

[[nodiscard]] int estimateCapacity(const ::DataPacketHeader &header,
                                   const std::chrono::seconds &memory)
{
    auto duration
        = std::max(0.0,
                   std::round( (header.nSamples - 1.)
                               /std::max(1, header.samplingRate)));
    std::chrono::seconds packetDuration{static_cast<int> (duration)};
    return std::max(10, static_cast<int> (1.5*memory.count()/duration)) + 1;
}

}

class TestDuplicateDataPacket::TestDuplicateDataPacketImpl
{
public:
    TestDuplicateDataPacketImpl() = default;
    TestDuplicateDataPacketImpl(const TestDuplicateDataPacketImpl &impl)
    {   
        *this = impl;
    }   
    void logBadData()
    {
        if (!mLogBadData){return;}
        auto now = std::chrono::high_resolution_clock::now();
        auto nowMuSeconds
           = std::chrono::time_point_cast<std::chrono::microseconds>
             (now).time_since_epoch();
        auto nowSeconds
            = std::chrono::duration_cast<std::chrono::seconds> (nowMuSeconds);
        {
        std::lock_guard<std::mutex> lockGuard(mMutex);
        if (nowSeconds >= mLastLogTime + mLogBadDataInterval)
        {
            if (!mDuplicateChannels.empty())
            {
                std::string message{"Duplicate packets detected for:"};
                for (const auto &channel : mDuplicateChannels)
                {
                    message = message + " " + channel;
                }
                spdlog::info(message);
                mDuplicateChannels.clear();
                mLastLogTime = nowSeconds;
            }
            if (!mBadTimingChannels.empty())
            {
                std::string message{"Bad timing detected for:"};
                for (const auto &channel : mBadTimingChannels)
                {
                    message = message + " " + channel;
                }
                spdlog::info(message);  
                mBadTimingChannels.clear();
                mLastLogTime = nowSeconds;
            }
        }
        }
    }
    [[nodiscard]] bool allow(const ::DataPacketHeader &header) const
    {
#ifndef NDEBUG
        assert(!header.name.empty());
        assert(header.nSamples > 0);
#endif
        // Does this channel exist?
        auto circularBufferIndex = mCircularBuffers.find(header.name);
        bool firstExample{false};
        if (circularBufferIndex == mCircularBuffers.end())
        {
            int capacity = mCircularBufferSize;
            if (mEstimateCapacity)
            {
                capacity
                    = ::estimateCapacity(header,
                                         mCircularBufferDuration);
            }
            spdlog::info("Creating new circular buffer for: "
                       + header.name + " with capacity: "
                       + std::to_string(capacity));
            boost::circular_buffer<::DataPacketHeader>
                newCircularBuffer(capacity);
            newCircularBuffer.push_back(header);
            mCircularBuffers.insert(std::pair{header.name,
                                              std::move(newCircularBuffer)});
            // Can't be a a duplicate because its the first one
            return true;
        }
        // Now we should definitely be able to find the appropriate circular
        // buffer for this stream 
        circularBufferIndex = mCircularBuffers.find(header.name);
        if (circularBufferIndex == mCircularBuffers.end())
        {
            spdlog::warn(
                "Algorithm error - circular buffer doesn't exist for: "
               + header.name);
            return false;
        }
        // See if this header exists (exactly)
        auto headerIndex
            = std::find(circularBufferIndex->second.begin(),
                        circularBufferIndex->second.end(),
                        header);
        if (headerIndex != circularBufferIndex->second.end())
        {
            if (mLogBadData)
            {
                spdlog::debug("Detected duplicate for: "
                            + header.name);
                {
                std::lock_guard<std::mutex> lockGuard(mMutex);
                if (!mDuplicateChannels.contains(header.name))
                {
                    mDuplicateChannels.insert(header.name);
                }
                }
            }
            return false;
        }
        // Insert it (typically new stuff shows up)
        if (header.startTime > circularBufferIndex->second.back().endTime)
        {
            spdlog::debug("Inserting " + header.name
                        + " at end of circular buffer");
            circularBufferIndex->second.push_back(header);
            return true;
        }
        // If it is is really old and there's space then push to front
        if (header.endTime < circularBufferIndex->second.front().startTime)
        {
            if (!circularBufferIndex->second.full())
            {
                spdlog::debug("Inserting " + header.name 
                            + " at front of circular buffer");
                circularBufferIndex->second.push_front(header);
#ifndef NDEBUG
                assert(std::is_sorted(circularBufferIndex->second.begin(),
                                      circularBufferIndex->second.end(),
                       [](const ::DataPacketHeader &lhs, const ::DataPacketHeader &rhs)
                       {
                          return lhs.startTime < rhs.startTime;
                       }));
#endif
            }
            // Note, if the buffer is full then this packet is expired in the
            // eyes of the circular buffer.  
            return false;
        }
        // The packet is old.  We have to check for a GPS slip.
        for (const auto &streamHeader : circularBufferIndex->second)
        {
            if ((header.startTime >= streamHeader.startTime &&
                 header.startTime <= streamHeader.endTime) ||
                (header.endTime >= streamHeader.startTime &&
                 header.endTime <= streamHeader.endTime))
            {
                //std::cout << std::setprecision(16) << header.name << " " << streamHeader.name << " | " << header.startTime.count()*1.e-6 << " " << streamHeader.startTime.count()*1.e-6 << " | " 
                //<< header.endTime.count()*1.e-6 << " " << streamHeader.endTime.count()*1.e-6 << std::endl;
                if (mLogBadData)
                {
                    spdlog::info("Detected possible timing slip for: "
                               + header.name);
                    {
                    std::lock_guard<std::mutex> lockGuard(mMutex);
                    if (!mBadTimingChannels.contains(header.name))
                    {
                        mBadTimingChannels.insert(header.name);
                    }
                    }
                }
                return false;
            }
        }
        // This appears to be a valid (out-of-order) back-fill
        spdlog::debug("Inserting " + header.name
                    + " in circular buffer then sorting...");
        circularBufferIndex->second.push_back(header);
        std::sort(circularBufferIndex->second.begin(),
                  circularBufferIndex->second.end(),
                  [](const ::DataPacketHeader &lhs, const ::DataPacketHeader &rhs)
                  {
                      return lhs.startTime < rhs.startTime;
                  });
        return true;
    }
    TestDuplicateDataPacketImpl& operator=(const TestDuplicateDataPacketImpl &impl)
    {
        if (&impl == this){return *this;}
        {
        std::lock_guard<std::mutex> lockGuard(impl.mMutex);
        mCircularBuffers = impl.mCircularBuffers;
        mDuplicateChannels = impl.mDuplicateChannels;
        mBadTimingChannels = impl.mBadTimingChannels;
        mLastLogTime = impl.mLastLogTime; 
        }
        mLogBadDataInterval = impl.mLogBadDataInterval;
        mCircularBufferDuration = impl.mCircularBufferDuration;
        mCircularBufferSize = impl.mCircularBufferSize;
        mLogBadData = impl.mLogBadData;
        mEstimateCapacity = impl.mEstimateCapacity;
        return *this;
    }
//private:
    mutable std::mutex mMutex;
    mutable std::map<std::string, boost::circular_buffer<::DataPacketHeader>>
        mCircularBuffers;
    mutable std::set<std::string> mDuplicateChannels;
    mutable std::set<std::string> mBadTimingChannels;
    std::chrono::seconds mLogBadDataInterval{3600};
    std::chrono::seconds mLastLogTime{0};
    std::chrono::seconds mCircularBufferDuration{300};
    int mCircularBufferSize{100}; // ~3s packets 
    bool mLogBadData{true};
    bool mEstimateCapacity{false};
};

/// Constructor
TestDuplicateDataPacket::TestDuplicateDataPacket() :
    pImpl(std::make_unique<TestDuplicateDataPacketImpl> ())
{
}

/// Constructor
TestDuplicateDataPacket::TestDuplicateDataPacket(
    const int circularBufferSize,
    const std::chrono::seconds &logBadDataInterval) :
    pImpl(std::make_unique<TestDuplicateDataPacketImpl> ())
{
    if (circularBufferSize < 1)
    {
        throw std::invalid_argument("Circular buffer size must be positive");
    }
    pImpl->mCircularBufferSize = circularBufferSize; 
    pImpl->mLogBadDataInterval = logBadDataInterval;
    pImpl->mEstimateCapacity = false;
    pImpl->mLogBadData = pImpl->mLogBadDataInterval.count() >= 0 ? true : false;
}

/// Constructor
TestDuplicateDataPacket::TestDuplicateDataPacket(
    const std::chrono::seconds &circularBufferDuration,
    const std::chrono::seconds &logBadDataInterval) :
    pImpl(std::make_unique<TestDuplicateDataPacketImpl> ())
{
    if (circularBufferDuration.count() < 1)
    {
        throw std::invalid_argument(
           "Circular buffer duration must be positive");
    }
    pImpl->mCircularBufferDuration = circularBufferDuration;
    pImpl->mLogBadDataInterval = logBadDataInterval;
    pImpl->mEstimateCapacity = true;
    pImpl->mLogBadData = pImpl->mLogBadDataInterval.count() >= 0 ? true : false;
}

/// Copy constructor
TestDuplicateDataPacket::TestDuplicateDataPacket(
    const TestDuplicateDataPacket &sanitizer)
{
    *this = sanitizer;
}

/// Move constructor
TestDuplicateDataPacket::TestDuplicateDataPacket(
    TestDuplicateDataPacket &&sanitizer) noexcept
{
    *this = std::move(sanitizer);
}

/// Copy assignment
TestDuplicateDataPacket&
TestDuplicateDataPacket::operator=(const TestDuplicateDataPacket &sanitizer)
{
    if (&sanitizer == this){return *this;} 
    pImpl = std::make_unique<TestDuplicateDataPacketImpl> (*sanitizer.pImpl);
    return *this;
}

/// Move assignment
TestDuplicateDataPacket&
TestDuplicateDataPacket::operator=(TestDuplicateDataPacket &&sanitizer) noexcept
{
    if (&sanitizer == this){return *this;}
    pImpl = std::move(sanitizer.pImpl);
    return *this;
}

/// Reset the class
/*
void TestDuplicateDataPacket::clear() noexcept
{
    pImpl = std::make_unique<TestDuplicateDataPacketImpl> ();
}
*/

/// Destructor
TestDuplicateDataPacket::~TestDuplicateDataPacket() = default;

/// Allow this packet?
bool TestDuplicateDataPacket::allow(
    const US8::MessageFormats::Broadcasts::DataPacket &packet) const
{
    // Construct the trace header for the circular buffer
    ::DataPacketHeader header;
    try
    {
        header = ::DataPacketHeader {packet}; // Copy elision
    }
    catch (const std::exception &e)
    {
        spdlog::warn(
            "Failed to unpack dataPacketHeader.  Failed because: "
          + std::string {e.what()} + "; Not allowing...");
        return false;
    }
    bool allow{true};
    try
    {
        allow = pImpl->allow(header);
    }
    catch (const std::exception &e)
    {
        spdlog::warn("Failed to check packet because "  
                   + std::string {e.what()});
    }
    try
    {
        pImpl->logBadData();
    }
    catch (const std::exception &e)
    {
        spdlog::warn("Error: " + std::string {e.what()}
                   + " detected during logging expired data.");
    }
    return allow;
}
