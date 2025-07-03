#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <cmath>
#include <nlohmann/json.hpp>
#include "us8/messageFormats/broadcasts/dataPacket.hpp"
#include "private/isEmpty.hpp"

#define MESSAGE_TYPE "US8::MessageFormats::Broadcasts::DataPacket"
#define MESSAGE_VERSION "1.0.0"

using namespace US8::MessageFormats::Broadcasts;

namespace
{

std::string convertString(const std::string &s) 
{
    auto temp = s;
    temp.erase(std::remove(temp.begin(), temp.end(), ' '), temp.end());
    std::transform(temp.begin(), temp.end(), temp.begin(), ::toupper);
    return temp;
}

nlohmann::json toJSONObject(const DataPacket &packet)
{
    nlohmann::json obj;
    obj["messageType"] = packet.getMessageType();
    obj["messageVersion"] = packet.getMessageVersion();
    obj["network"] = packet.getNetwork();
    obj["station"] = packet.getStation();
    obj["channel"] = packet.getChannel();
    obj["locationCode"] = packet.getLocationCode();
    obj["startTime"] = packet.getStartTime().count();
    obj["samplingRate"] = packet.getSamplingRate();
    if (packet.haveSamplingRate() && packet.getNumberOfSamples() > 0)
    {
        obj["endTime"] = packet.getEndTime().count();
    }
    if (packet.getNumberOfSamples() > 0)
    {
        auto dataType = packet.getDataType();
        if (dataType == DataPacket::DataType::Integer32)
        {
            obj["dataType"] = "integer32";
            obj["data"] = packet.getData<int32_t> ();
        }
        else if (dataType == DataPacket::DataType::Double)
        {
            obj["dataType"] = "double";
            obj["data"] = packet.getData<double> ();
        }
        else if (dataType == DataPacket::DataType::Integer64)
        {
            obj["dataType"] = "integer64";
            obj["data"] = packet.getData<int64_t> ();
        }
        else if (dataType == DataPacket::DataType::Float)
        {
            obj["dataType"] = "float";
            obj["data"] = packet.getData<float> ();
        }
        else
        {
#ifndef NDEBUG
            assert(false);
#else
            throw std::runtime_error("Unhandled data type");
#endif
        }
    }
    return obj;
}

DataPacket objectToDataPacket(const nlohmann::json &obj)
{
    DataPacket packet;
    if (obj["messageType"] != packet.getMessageType())
    {
        throw std::invalid_argument("Message has invalid message type");
    }
    // Essential stuff
    packet.setNetwork(obj["network"].get<std::string> ());
    packet.setStation(obj["station"].get<std::string> ());
    packet.setChannel(obj["channel"].get<std::string> ());
    packet.setLocationCode(obj["locationCode"].get<std::string> ());
    packet.setSamplingRate(obj["samplingRate"].get<double> ());
    auto startTime = obj["startTime"].get<int64_t> ();
    std::chrono::microseconds startTimeMuS{startTime};
    packet.setStartTime(startTimeMuS);

    std::vector<double> data = obj["data"]; //.get<std::vector<T>>
    if (!data.empty()){packet.setData(std::move(data));}
    return std::move(packet);
}

DataPacket fromCBORMessage(const uint8_t *message,
                           const size_t length)
{
    auto obj = nlohmann::json::from_cbor(message, message + length);
    return std::move(::objectToDataPacket(obj));
}

}

class DataPacket::DataPacketImpl
{
public:
    [[nodiscard]] int size() const
    {   
        if (mDataType == DataPacket::DataType::Unknown)
        {
            return 0;
        }
        else if (mDataType == DataPacket::DataType::Integer32)
        {   
            return static_cast<int> (mInteger32Data.size());
        }   
        else if (mDataType == DataPacket::DataType::Float)
        {
            return static_cast<int> (mFloatData.size());
        }
        else if (mDataType == DataPacket::DataType::Double)
        {
            return static_cast<int> (mDoubleData.size());
        }
        else if (mDataType == DataPacket::DataType::Integer64)
        {   
            return static_cast<int> (mInteger64Data.size());
        }   
#ifndef NDEBUG
        assert(false);
#else
        throw std::runtime_error("Unhandled data type in packet impl size");
#endif
    }
    void clearData()
    {
        mInteger32Data.clear();
        mInteger64Data.clear();
        mFloatData.clear();
        mDoubleData.clear();
        mDataType = DataPacket::DataType::Unknown;
    }
    void setData(std::vector<int> &&data)
    {
        if (data.empty()){return;}
        clearData();
        mInteger32Data = std::move(data);
        mDataType = DataPacket::DataType::Integer32;
        updateEndTime();
    }
    void setData(std::vector<float> &&data)
    {
        if (data.empty()){return;}
        clearData();
        mFloatData = std::move(data);
        mDataType = DataPacket::DataType::Float;
        updateEndTime();
    }
    void setData(std::vector<double> &&data)
    {
        if (data.empty()){return;}
        clearData();
        mDoubleData = std::move(data);
        mDataType = DataPacket::DataType::Double;
        updateEndTime();
    }
    void setData(std::vector<int64_t> &&data)
    {
        if (data.empty()){return;}
        clearData();
        mInteger64Data = std::move(data);
        mDataType = DataPacket::DataType::Integer64;
        updateEndTime();
    }
    void updateEndTime()
    {
        mEndTimeMicroSeconds = mStartTimeMicroSeconds;
        auto nSamples = size();  
        if (nSamples > 0 && mSamplingRate > 0)
        {
            auto traceDuration
                = std::round( ((nSamples - 1)/mSamplingRate)*1000000 );
            auto iTraceDuration = static_cast<int64_t> (traceDuration);
            std::chrono::microseconds traceDurationMuS{iTraceDuration};
            mEndTimeMicroSeconds = mStartTimeMicroSeconds + traceDurationMuS;
        }
    }
    std::string mNetwork;
    std::string mStation;
    std::string mChannel;
    std::string mLocationCode;
    std::vector<int> mInteger32Data;
    std::vector<int64_t> mInteger64Data;
    std::vector<float> mFloatData;
    std::vector<double> mDoubleData;
    std::chrono::microseconds mStartTimeMicroSeconds{0};
    std::chrono::microseconds mEndTimeMicroSeconds{0};
    double mSamplingRate{0};
    DataPacket::DataType mDataType{DataPacket::DataType::Unknown};
};

/// Clear class
void DataPacket::clear() noexcept
{
    pImpl->clearData();
    pImpl->mNetwork.clear();
    pImpl->mStation.clear();
    pImpl->mChannel.clear();
    pImpl->mLocationCode.clear();
    constexpr std::chrono::microseconds zeroMuS{0};
    pImpl->mStartTimeMicroSeconds = zeroMuS;
    pImpl->mEndTimeMicroSeconds = zeroMuS;
    pImpl->mSamplingRate = 0;
}

/// C'tor
DataPacket::DataPacket() :
    IMessage(),
    pImpl(std::make_unique<DataPacketImpl> ())
{
}

/// Copy c'tor 
DataPacket::DataPacket(const DataPacket &packet)
{
    *this = packet;
}

/// Move c'tor
DataPacket::DataPacket(DataPacket &&packet) noexcept
{
    *this = std::move(packet);
}

/// Copy assignment
DataPacket& DataPacket::operator=(const DataPacket &packet)
{
    if (&packet == this){return *this;}
    pImpl = std::make_unique<DataPacketImpl> (*packet.pImpl);
    return *this;
}

/// Move assignment
DataPacket& DataPacket::operator=(DataPacket &&packet) noexcept
{
    if (&packet == this){return *this;}
    pImpl = std::move(packet.pImpl);
    return *this;
}

/// Destructor
DataPacket::~DataPacket() = default;

/// Network
void DataPacket::setNetwork(const std::string &network)
{
    if (::isEmpty(network)){throw std::invalid_argument("Network is empty");}
    pImpl->mNetwork = ::convertString(network);
}

std::string DataPacket::getNetwork() const
{
    if (!haveNetwork()){throw std::runtime_error("Network not set yet");}
    return pImpl->mNetwork;
}

bool DataPacket::haveNetwork() const noexcept
{
    return !pImpl->mNetwork.empty();
}

/// Station
void DataPacket::setStation(const std::string &station)
{
    if (::isEmpty(station)){throw std::invalid_argument("Station is empty");}
    pImpl->mStation = ::convertString(station);
}

std::string DataPacket::getStation() const
{
    if (!haveStation()){throw std::runtime_error("Station not set yet");}
    return pImpl->mStation;
}

bool DataPacket::haveStation() const noexcept
{
    return !pImpl->mStation.empty();
}

/// Channel
void DataPacket::setChannel(const std::string &channel)
{
    if (::isEmpty(channel)){throw std::invalid_argument("Channel is empty");}
    pImpl->mChannel = ::convertString(channel);
}

std::string DataPacket::getChannel() const
{
    if (!haveChannel()){throw std::runtime_error("Channel not set yet");}
    return pImpl->mChannel;
}

bool DataPacket::haveChannel() const noexcept
{
    return !pImpl->mChannel.empty();
}

/// Location code
void DataPacket::setLocationCode(const std::string &location)
{
    if (::isEmpty(location)){throw std::invalid_argument("Location is empty");}
    pImpl->mLocationCode = ::convertString(location);
}

std::string DataPacket::getLocationCode() const
{
    if (!haveLocationCode())
    {
        throw std::runtime_error("Location code not set yet");
    }
    return pImpl->mLocationCode;
}

bool DataPacket::haveLocationCode() const noexcept
{
    return !pImpl->mLocationCode.empty();
}

/// Sampling rate
void DataPacket::setSamplingRate(const double samplingRate) 
{
    if (samplingRate <= 0)
    {
        throw std::invalid_argument("samplingRate = "
                                  + std::to_string(samplingRate)
                                  + " must be positive");
    }
    pImpl->mSamplingRate = samplingRate;
    pImpl->updateEndTime();
}

double DataPacket::getSamplingRate() const
{
    if (!haveSamplingRate()){throw std::runtime_error("Sampling rate not set");}
    return pImpl->mSamplingRate;
}

bool DataPacket::haveSamplingRate() const noexcept
{
    return (pImpl->mSamplingRate > 0);     
}

/// Number of samples
int DataPacket::getNumberOfSamples() const noexcept
{
    return pImpl->size();
}

/// Start time
void DataPacket::setStartTime(const double startTime) noexcept
{
    auto iStartTimeMuS = static_cast<int64_t> (std::round(startTime*1.e6));
    std::chrono::microseconds startTimeMuS{iStartTimeMuS};
    setStartTime(startTimeMuS);
}

void DataPacket::setStartTime(
    const std::chrono::microseconds &startTime) noexcept
{
    pImpl->mStartTimeMicroSeconds = startTime;
    pImpl->updateEndTime();
}

std::chrono::microseconds DataPacket::getStartTime() const noexcept
{
    return pImpl->mStartTimeMicroSeconds;
}

std::chrono::microseconds DataPacket::getEndTime() const
{
    if (!haveSamplingRate())
    {   
        throw std::runtime_error("Sampling rate note set");
    }   
    if (getNumberOfSamples() < 1)
    {   
        throw std::runtime_error("No samples in signal");
    }   
    return pImpl->mEndTimeMicroSeconds;
}

/// Sets the data
template<typename U>
void DataPacket::setData(std::vector<U> &&x)
{
    pImpl->setData(std::move(x));
    pImpl->updateEndTime();
}

template<typename U>
void DataPacket::setData(const std::vector<U> &x)
{
    auto xWork = x;
    setData(std::move(xWork));
}

template<typename U>
void DataPacket::setData(const int nSamples, const U *x)
{
    // Invalid
    if (nSamples < 0){throw std::invalid_argument("nSamples not positive");}
    if (x == nullptr){throw std::invalid_argument("x is NULL");}
    std::vector<U> data(nSamples);
    std::copy(x, x + nSamples, data.begin());
    setData(std::move(data));
}

/// Gets the data
template<typename U>
std::vector<U> DataPacket::getData() const noexcept
{
    std::vector<U> result;
    auto nSamples = getNumberOfSamples();
    if (nSamples < 1){return result;}
    result.resize(nSamples);
    auto dataType = getDataType();
    if (dataType == DataType::Integer32)
    {   
        std::copy(pImpl->mInteger32Data.begin(),
                  pImpl->mInteger32Data.end(),
                  result.begin());
    }   
    else if (dataType == DataType::Float)
    {   
        std::copy(pImpl->mFloatData.begin(),
                  pImpl->mFloatData.end(),
                  result.begin());
    }   
    else if (dataType == DataType::Double)
    {   
        std::copy(pImpl->mDoubleData.begin(),
                  pImpl->mDoubleData.end(),
                  result.begin());
    }   
    else if (dataType == DataType::Integer64)
    {   
        std::copy(pImpl->mInteger64Data.begin(),
                  pImpl->mInteger64Data.end(),
                  result.begin());
    }   
    else
    {   
#ifndef NDEBUG
        assert(false);
#endif
        constexpr U zero{0};
        std::fill(result.begin(), result.end(), zero); 
    }   
    return result;
}

/*
const std::vector<double> &DataPacket::getDataReference() const noexcept
{
    return pImpl->mData;
}
*/

const void* DataPacket::getDataPointer() const noexcept
{
    if (getNumberOfSamples() < 1){return nullptr;}
    auto dataType = getDataType();
    if (dataType == DataType::Integer32)
    {   
        return pImpl->mInteger32Data.data();
    }   
    else if (dataType == DataType::Float)
    {   
        return pImpl->mFloatData.data();
    }   
    else if (dataType == DataType::Double)
    {   
        return pImpl->mDoubleData.data();
    }   
    else if (dataType == DataType::Integer64)
    {   
        return pImpl->mInteger64Data.data();
    }   
    else if (dataType  == DataType::Unknown)
    {   
        return nullptr;
    }   
#ifndef NDEBUG
    else 
    {   
        assert(false);
    }   
#endif
    return nullptr;
}

/// Message format
std::string DataPacket::getMessageType() const noexcept
{
    return MESSAGE_TYPE;
}

/// Copy this class
std::unique_ptr<US8::MessageFormats::IMessage> DataPacket::clone() const
{
    std::unique_ptr<US8::MessageFormats::IMessage> result
        = std::make_unique<DataPacket> (*this);
    return result;
}

/// Create an instance of this class 
std::unique_ptr<US8::MessageFormats::IMessage>
    DataPacket::createInstance() const noexcept
{
    std::unique_ptr<US8::MessageFormats::IMessage> result
        = std::make_unique<DataPacket> (); 
    return result;
}

///  Convert message
std::string DataPacket::serialize() const
{
    auto obj = ::toJSONObject(*this);
    auto message = nlohmann::json::to_cbor(obj);
    std::string result(message.begin(), message.end());
    return result;
}

void DataPacket::deserialize(const std::string &message)
{
    if (message.empty()){throw std::invalid_argument("Message is empty");}
    deserialize(message.data(), message.size());   
}

void DataPacket::deserialize(const char *messageIn, const size_t length)
{
    if (length == 0){throw std::invalid_argument("No data");}
    if (messageIn == nullptr)
    {   
        throw std::invalid_argument("message is null");
    }
    auto message = reinterpret_cast<const uint8_t *> (messageIn);
    *this = std::move(::fromCBORMessage(message, length));
}

/// Data type
DataPacket::DataType DataPacket::getDataType() const noexcept
{
    return pImpl->mDataType;
}

/// Message version
std::string DataPacket::getMessageVersion() const noexcept
{
    return MESSAGE_VERSION;
}

///--------------------------------------------------------------------------///
///                               Template Instantiation                     ///
///--------------------------------------------------------------------------///
template void US8::MessageFormats::Broadcasts::DataPacket::setData(std::vector<double> &&);
template void US8::MessageFormats::Broadcasts::DataPacket::setData(std::vector<float> &&);
template void US8::MessageFormats::Broadcasts::DataPacket::setData(std::vector<int> &&);
template void US8::MessageFormats::Broadcasts::DataPacket::setData(std::vector<int64_t> &&);

template void US8::MessageFormats::Broadcasts::DataPacket::setData(const int, const double *);
template void US8::MessageFormats::Broadcasts::DataPacket::setData(const int, const float *);
template void US8::MessageFormats::Broadcasts::DataPacket::setData(const int, const int *);
template void US8::MessageFormats::Broadcasts::DataPacket::setData(const int, const int64_t *);
