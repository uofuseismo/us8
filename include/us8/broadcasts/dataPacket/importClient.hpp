#ifndef US8_BROADCASTS_DATA_PACKET_IMPORT_CLIENT_HPP
#define US8_BROADCASTS_DATA_PACKET_IMPORT_CLIENT_HPP
#include <functional>
#include <memory>
#include <vector>
namespace US8::MessageFormats::Broadcasts
{
 class DataPacket;
}
namespace US8::Broadcasts::DataPacket
{
class IImportClient
{
public:
    enum class Type
    {
        SEEDLink
    };
public:
    /// @brief Construtor with callback.
    explicit IImportClient(const std::function<void (US8::MessageFormats::Broadcasts::DataPacket &&packet)> &callback);
    /// @brief Destructor.
    virtual ~IImportClient();
    /// @brief Connects the client to the data source.
    virtual void connect() = 0;
    /// @brief Starts the acquisition.
    virtual void start() = 0;
    /// @brief Terminates the acquisition.
    virtual void stop() = 0;
    /// @result The client type.
    virtual Type getType() const noexcept = 0;
    /// @result True indicates the client is ready to receive 
    ///         data packets.
    [[nodiscard]] virtual bool isInitialized() const noexcept = 0;
    /// @result True indicates the client is connected.
    [[nodiscard]] virtual bool isConnected() const noexcept = 0;
    /// @brief Passes the packets read from the client to the callback.
    /// @param[in,out] packets  The packets to send to the callback.
    ///                         On exit,  the behavior of packets is undefined.
    virtual void operator()(std::vector<US8::MessageFormats::Broadcasts::DataPacket> &&packets);
    /// @brief Passes the packet read from the client to the callback.
    /// @param[in,out] packet  The packet to send to the callback.
    ///                        On exit, packet's behavior is undefined.
    virtual void operator()(US8::MessageFormats::Broadcasts::DataPacket &&packet);
    /// @brief Constructor.
    IImportClient() = delete;
private:
    class IImportClientImpl;
    std::unique_ptr<IImportClientImpl> pImpl;
};
}
#endif
