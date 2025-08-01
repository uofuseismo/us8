#ifndef US8_BROADCASTS_DATA_PACKET_PUBLISHER_OPTIONS_HPP
#define US8_BROADCASTS_DATA_PACKET_PUBLISHER_OPTIONS_HPP
#include <chrono>
#include <string>
#include <memory>
namespace US8::Broadcasts::DataPacket
{
/// @brief Defines the options for a data broadcast publisher.
/// @copyright Ben Baker (University of Utah) distributed under the MIT
///            NO AI license.
class PublisherOptions
{
public:
    /// @brief Constructs the publisher options.
    /// @param[in] endPoint  The endpoint to which to connect - e.g.,
    ///                      tcp://127.0.0.1:5555.
    explicit PublisherOptions(const std::string &endPoint);
    /// @brief Copy constructor.
    PublisherOptions(const PublisherOptions &options);
    /// @brief Move constructor.
    PublisherOptions(PublisherOptions &&options) noexcept;

    /// @name Required Parameters
    /// @{

    /// @result The end point to which to connect and send messages.
    [[nodiscard]] std::string getEndPoint() const;
 
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

    /// @brief Sets a hard limits on the number of outbound messages.  If this
    ///        limit is hit then the socket may block or drop packets.
    /// @param[in] highWaterMark  Limit on the number of outbound messages.
    ///                           Note, 0 sets this to "no limit".
    /// @throw std::invalid_argument if this is negative.
    void setHighWaterMark(int highWaterMark);
    /// @result The high water mark.
    [[nodiscard]] int getHighWaterMark() const noexcept;
    /// @}

    ~PublisherOptions();
    PublisherOptions& operator=(const PublisherOptions &options);
    PublisherOptions& operator=(PublisherOptions &&options) noexcept;
private:
    class PublisherOptionsImpl;
    std::unique_ptr<PublisherOptionsImpl> pImpl; 
};
}
#endif    
