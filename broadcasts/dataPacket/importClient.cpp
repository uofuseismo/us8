#include <spdlog/spdlog.h>
#include "us8/broadcasts/dataPacket/importClient.hpp"
#include "us8/messageFormats/broadcasts/dataPacket.hpp"

using namespace US8::Broadcasts::DataPacket;

class IImportClient::IImportClientImpl
{
public:
    explicit IImportClientImpl(
        const std::function<void (US8::MessageFormats::Broadcasts::DataPacket &&packet)> &callback) :
        mCallback(callback)
    {
    }
    std::function<void (US8::MessageFormats::Broadcasts::DataPacket &&packets)> mCallback;
};

IImportClient::IImportClient(
    const std::function<void (US8::MessageFormats::Broadcasts::DataPacket &&packet)> &callback) :
    pImpl(std::make_unique<IImportClientImpl> (callback))
{
}

IImportClient::~IImportClient() = default;

/// Applies the callback
void IImportClient::operator()(
    std::vector<US8::MessageFormats::Broadcasts::DataPacket> &&packets)
{
    for (auto &packet : packets)
    {
        this->operator()(std::move(packet));
    }
}

/// Applies the callback to a packet
void IImportClient::operator()
   (US8::MessageFormats::Broadcasts::DataPacket &&packet)
{
   pImpl->mCallback(std::move(packet));
}
