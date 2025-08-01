#ifndef US8_BROADCASTS_DATA_PACKET_PUBLISHER_HPP
#define US8_BROADCASTS_DATA_PACKET_PUBLISHER_HPP
#include <memory>
namespace US8::MessageFormats::Broadcasts
{
 class DataPacket;
}
namespace US8::Broadcasts::DataPacket
{
 class PublisherOptions;
}
namespace US8::Broadcasts::DataPacket
{
/// @brief Pubblishes a data packet to a broadcast.
/// @copyright Ben Baker (University of Utah) distributed under the MIT
///            NO AI license.
class Publisher
{
public:
    /// @brief Creates the publisher from the given options.
    explicit Publisher(const PublisherOptions &options);
    /// @brief Publishes a data packet.
    void send(const US8::MessageFormats::Broadcasts::DataPacket &dataPacket);
    /// @brief Destructor.
    ~Publisher();

    void operator()(const US8::MessageFormats::Broadcasts::DataPacket &dataPacket);
 
    Publisher() = delete;
    Publisher(const Publisher &) = delete;
    Publisher(Publisher &&publisher) noexcept = delete;
    Publisher& operator=(const Publisher &) = delete;
    Publisher& operator=(Publisher &&) noexcept = delete;
private:
    class PublisherImpl;
    std::unique_ptr<PublisherImpl> pImpl;
};
}
#endif

