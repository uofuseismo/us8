#ifndef US8_BROADCASTS_SANITIZER_TEST_FUTURE_DATA_PACKET_HPP
#define US8_BROADCASTS_SANITIZER_TEST_FUTURE_DATA_PACKET_HPP
#include <chrono>
#include <string>
#include <memory>
namespace US8::MessageFormats::Broadcasts
{
class DataPacket;
}
namespace US8::Broadcasts::DataPacket::Sanitizer
{
/// @brief Tests whether or not a packet contains data from the future.  This
///        indicates that there is a timing error.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class TestFutureDataPacket
{
public:
    /// @brief Constructs a future time checker with a default max future time
    ///        of 0 (which after data transmission and scraping from some type
    ///        of import device is pretty conservative).  Sensors sending
    ///        packets from the future will be logged every hour.
    TestFutureDataPacket();
    /// @param[in] maxFutureTime  If the time of the last sample in the packet
    ///                           exceeds now + maxFutureTime then the packet
    ///                           is rejected for having bad data.
    /// @param[in] logBadDataInterval  If this is positive then this will
    ///                                log flagged channels at approximately
    ///                                this interval.
    TestFutureDataPacket(const std::chrono::microseconds &maxFutureTime,
                         const std::chrono::seconds &logBadDataInterval);
    /// @brief Copy constructor.
    TestFutureDataPacket(const TestFutureDataPacket &testFutureDataPacket);
    /// @brief Move constructor.
    TestFutureDataPacket(TestFutureDataPacket &&testFutureDataPacket) noexcept;

    /// @result True indicates the data packet does not appear to have any future data.
    [[nodiscard]] bool allow(const US8::MessageFormats::Broadcasts::DataPacket &packet) const;
    /// @result True indicates the data packet does not appear to have any future data.
    [[nodiscard]] bool operator()(const US8::MessageFormats::Broadcasts::DataPacket &packet) const;

    /// @brief Destructor.
    ~TestFutureDataPacket();
    /// @brief Copy assignment.
    TestFutureDataPacket& operator=(const TestFutureDataPacket &testFutureDataPacket);
    /// @brief Move constructor.
    TestFutureDataPacket& operator=(TestFutureDataPacket &&testFutureDataPacket) noexcept;
private:
    class TestFutureDataPacketImpl;
    std::unique_ptr<TestFutureDataPacketImpl> pImpl;
};
}
#endif
