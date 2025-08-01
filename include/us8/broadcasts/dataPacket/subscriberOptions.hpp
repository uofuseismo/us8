#ifndef US8_BROADCASTS_DATA_PACKET_SUBSCRIBER_OPTIONS_HPP
#define US8_BROADCASTS_DATA_PACKET_SUBSCRIBER_OPTIONS_HPP
#include <functional>
#include <string>
#include <chrono>
#include <optional>
#include <memory>
namespace US8::MessageFormats::Broadcasts
{
 class DataPacket;
}
namespace US8::Broadcasts::DataPacket
{
/// @brief Defines the options for a data broadcast subscriber.
/// @copyright Ben Baker (University of Utah) distributed under the MIT
///            NO AI license.
class SubscriberOptions
{
public:
    /// @brief Constructs the subscriber options.
    /// @param[in] endPoint  The endpoint to which to connect - e.g.,
    ///                      tcp://127.0.0.1:5555.
    /// @param[in] callback  The callback used to processed messages. 
    SubscriberOptions(const std::string &endPoint,
                      const std::function<void (US8::MessageFormats::Broadcasts::DataPacket &&)> &callabck);
    /// @brief Copy constructor.
    SubscriberOptions(const SubscriberOptions &options);
    /// @brief Move constructor.
    SubscriberOptions(SubscriberOptions &&options) noexcept;

    /// @name Required Parameters
    /// @{

    /// @result The end point. 
    [[nodiscard]] std::string getEndPoint() const;
 
    /// @result The callback for handling the packet.
    [[nodiscard]] std::function<void (US8::MessageFormats::Broadcasts::DataPacket &&)> getCallback() const;
    /// @}

    /// @name Optional Parameters
    /// @{

    /// @brief The listening thread will timeout after this interval and then
    ///        check for other commands.
    /// @param[in] timeOut   The thread's timeout.  Note, if this is negative 
    ///                      then the listening thread will wait forever to
    ///                      receive a packet.  Additionally, if this is zero
    ///                      then the listening thread may burn a lot of compute
    ///                      cycles in a tight receive loop.
    void setTimeOut(const std::chrono::milliseconds &timeOut) noexcept;
    /// @result The receive time out.
    [[nodiscard]] std::chrono::milliseconds getTimeOut() const noexcept; 

    /// @brief Sets a hard limits on the number of inbound messages.  If this
    ///        is hit then the socket may block or drop packets.
    /// @param[in] highWaterMark  Limit on the number of inbound messages.
    ///                           Note, 0 sets this to "no limit".
    /// @throw std::invalid_argument if this is negative.
    void setHighWaterMark(int highWaterMark);
    /// @result The high water mark.
    [[nodiscard]] int getHighWaterMark() const noexcept;

    /// @brief The subscriber can periodically post performance statistics 
    ///        at this interval.
    /// @param[in] interval  The logging interval.  Note, if this is negative
    ///                      then no logging will be performed.
    void setLoggingInterval(const std::chrono::seconds &interval) noexcept;
    /// @result The logging interval.
    [[nodiscard]] std::chrono::seconds getLoggingInterval() const noexcept;
    /// @}

    ~SubscriberOptions();
    SubscriberOptions& operator=(const SubscriberOptions &options);
    SubscriberOptions& operator=(SubscriberOptions &&options) noexcept;
private:
    class SubscriberOptionsImpl;
    std::unique_ptr<SubscriberOptionsImpl> pImpl; 
};
}
#endif    
