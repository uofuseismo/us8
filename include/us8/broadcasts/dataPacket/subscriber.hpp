#ifndef US8_BROADCASTS_DATA_PACKET_SUBSCRIBER_HPP
#define US8_BROADCASTS_DATA_PACKET_SUBSCRIBER_HPP
#include <memory>
namespace US8::Broadcasts::DataPacket
{
 class SubscriberOptions;
}
namespace US8::Broadcasts::DataPacket
{
/// @brief Subscribes to a data packet broadcast.
/// @copyright Ben Baker (University of Utah) distributed under the MIT
///            NO AI license.
class Subscriber
{
public:
    /// @brief Creates the subscriber from the given options.
    explicit Subscriber(const SubscriberOptions &options);

    /// @brief Starts a thread that listens to a data packet broadcast
    ///        and propagetes packets as defined by the callback.
    void start();
    /// @brief Stops the listening thread.
    void stop();

    /// @brief Destructor.
    ~Subscriber();
 
    Subscriber() = delete;
    Subscriber(const Subscriber &) = delete;
    Subscriber(Subscriber &&subscriber) noexcept = delete;
    Subscriber& operator=(const Subscriber &) = delete;
    Subscriber& operator=(Subscriber &&) noexcept = delete;
private:
    class SubscriberImpl;
    std::unique_ptr<SubscriberImpl> pImpl;
};
}
#endif
