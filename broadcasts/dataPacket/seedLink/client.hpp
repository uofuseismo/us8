#ifndef US8_DATA_CLIENT_DATA_PACKET_SEED_LINK_HPP
#define US8_DATA_CLIENT_DATA_PACKET_SEED_LINK_HPP
#include <us8/broadcasts/dataPacket/client.hpp>
namespace US8::MessageFormats::Broadcasts
{
 class DataPacket;
}
namespace US8::Broadcasts::DataPacket::SEEDLink
{
 class ClientOptions;
}
namespace US8::Broadcasts::DataPacket::SEEDLink
{
class Client : public US8::Broadcasts::DataPacket::IClient
{
public:
    Client() = delete;
    Client(const std::function<void (US8::MessageFormats::Broadcasts::DataPacket &&packets)> &callback,
           const ClientOptions &options);
    ~Client() override;
    void connect() final;
    void start() final;
    void stop() final;
    [[nodiscard]] bool isInitialized() const noexcept final;
    [[nodiscard]] bool isConnected() const noexcept final;
    [[nodiscard]] US8::Broadcasts::DataPacket::IClient::Type getType() const noexcept final;
private:
    class ClientImpl;
    std::unique_ptr<ClientImpl> pImpl;
};
}
#endif
