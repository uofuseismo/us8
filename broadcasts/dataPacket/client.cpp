#include <spdlog/spdlog.h>
#include "us8/broadcasts/dataPacket/client.hpp"
#include "us8/messageFormats/broadcasts/dataPacket.hpp"

using namespace US8::Broadcasts::DataPacket;

class IClient::IClientImpl
{
public:
    explicit IClientImpl(
        const std::function<void (US8::MessageFormats::Broadcasts::DataPacket &&packet)> &callback) :
        mCallback(callback)
    {
    }
    std::function<void (US8::MessageFormats::Broadcasts::DataPacket &&packets)> mCallback;
};

IClient::IClient(
    const std::function<void (US8::MessageFormats::Broadcasts::DataPacket &&packet)> &callback) :
    pImpl(std::make_unique<IClientImpl> (callback))
{
}

IClient::~IClient() = default;

/// Applies the callback
void IClient::operator()(
    std::vector<US8::MessageFormats::Broadcasts::DataPacket> &&packets)
{
    for (auto &packet : packets)
    {
        this->operator()(std::move(packet));
    }
}

/// Applies the callback to a packet
void IClient::operator()(US8::MessageFormats::Broadcasts::DataPacket &&packet)
{
   pImpl->mCallback(std::move(packet));
}
