#ifndef US8_BROADCASTS_SANITIZER_TEST_EXPIRED_DATA_PACKET_HPP
#define US8_BROADCASTS_SANITIZER_TEST_EXPIRED_DATA_PACKET_HPP
#include <chrono>
#include <string>
#include <memory>
namespace US8::MessageFormats::Broadcasts
{
class DataPacket;
}
namespace US8::Broadcasts::DataPacket::Sanitizer
{
/// @brief Tests whether or not a packet contains data that has expired.  This
///        indicates that a backfill is from too far back to be useful or 
///        there is a timing error.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class TestExpiredDataPacket
{
public:
    /// @brief Constructs an expired time checker with a default max past time
    ///        of 90 days (which is pretty conserative given that most 
    ///        stations at UUSS can only backfill for a few weeks).
    ///        Sensors sending packets that have samples at older timer
    ///        be flagged and logged every hour.
    TestExpiredDataPacket();
    /// @param[in] maxExpiredTime  If the time of the first sample in the packet
    ///                            is less than now - maxExpiredTime then the
    ///                            packet is rejected for having bad data.
    /// @param[in] logBadDataInterval  If this is positive then this will
    ///                                log flagged channels at approximately
    ///                                this interval.
    TestExpiredDataPacket(const std::chrono::microseconds &maxExpiredTime,
                      const std::chrono::seconds &logBadDataInterval);
    /// @brief Copy constructor.
    TestExpiredDataPacket(const TestExpiredDataPacket &testExpiredPacket);
    /// @brief Move constructor.
    TestExpiredDataPacket(TestExpiredDataPacket &&testExpiredPacket) noexcept;

    /// @param[in] packet  The packet to test.
    /// @result True indicates the data does not appear to have any expired
    ///         data.
    [[nodiscard]] bool allow(const US8::MessageFormats::Broadcasts::DataPacket &packet) const;

    /// @brief Destructor.
    ~TestExpiredDataPacket();
    /// @brief Copy assignment.
    TestExpiredDataPacket& operator=(const TestExpiredDataPacket &testExpiredPacket);
    /// @brief Move constructor.
    TestExpiredDataPacket& operator=(TestExpiredDataPacket &&testExpiredPacket) noexcept;
private:
    class TestExpiredDataPacketImpl;
    std::unique_ptr<TestExpiredDataPacketImpl> pImpl;
};
}
#endif
