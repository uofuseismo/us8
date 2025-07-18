#ifndef US8_BROADCASTS_SANITIZER_TEST_DUPLICATE_DATA_PACKET_HPP
#define US8_BROADCASTS_SANITIZER_TEST_DUPLICATE_DATA_PACKET_HPP
#include <chrono>
#include <string>
#include <memory>
namespace US8::MessageFormats::Broadcasts
{
class DataPacket;
}
namespace US8::Broadcasts::DataPacket::Sanitizer
{
/// @brief Tests whether or not this packet may have been previously processed
///        This works by comparing the packet's header (start and end time)
///        to previous packets collected in a circular buffer.  Additionally,
///        it can detect GPS slips.  For example, if an older packet arrives
///        with times contained between earlier process packets then it is also
///        rejected.
///
///        Note, if the packet is very old then it's previously processed
///        counterpart may have been purged from the circular buffer.  The
///        packet will then be allowed.  In this instance, this is okay
///        because the database will detect a conflict on the start time
///        and default to an earlier packet.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class TestDuplicateDataPacket
{
public:
    /// @brief Constructs a duplicate packet checker.  This constructs circular
    ///        buffers with size 100.  At about 2 seconds per packet this 
    ///        represents data for a little over three minutes which should
    ///        be sufficient for alternate telemetry routes.  Sensors sending
    ///        duplicate packets will be flagged and logged every hour.
    TestDuplicateDataPacket();
    /// @param[in] circularBufferSize  The number of packets retained in the
    ///                                circular buffer.
    /// @param[in] logBadDataInterval  If this is positive then this will
    ///                                log flagged channels at approximately
    ///                                this interval.
    TestDuplicateDataPacket(const int circularBufferSize,
                            const std::chrono::seconds &logBadDataInterval);
    /// @param[in] circularBufferDuration  The approximate temporal duration
    ///                                    of the circular buffer.
    /// @param[in] logBadDataInterval  If this is postiive then this iwll
    ///                                log flagged channels at approximately
    ///                                this interval.
    /// @note This will estimate the capacity based on the size a sensor's
    ///       first packet.
    TestDuplicateDataPacket(const std::chrono::seconds &circularBufferDuration,
                            const std::chrono::seconds &logBadDataInterval);
    /// @brief Copy constructor.
    TestDuplicateDataPacket(const TestDuplicateDataPacket &testDuplicatePacket);
    /// @brief Move constructor.
    TestDuplicateDataPacket(TestDuplicateDataPacket &&testDuplicatePacket) noexcept;

    /// @param[in] packet  The packet to test.
    /// @result True indicates the data does not appear to have been processed.
    [[nodiscard]] bool allow(const US8::MessageFormats::Broadcasts::DataPacket &packet) const;

    /// @brief Destructor.
    ~TestDuplicateDataPacket();
    /// @brief Copy assignment.
    TestDuplicateDataPacket& operator=(const TestDuplicateDataPacket &testDuplicatePacket);
    /// @brief Move constructor.
    TestDuplicateDataPacket& operator=(TestDuplicateDataPacket &&testDuplicatePacket) noexcept;
private:
    class TestDuplicateDataPacketImpl;
    std::unique_ptr<TestDuplicateDataPacketImpl> pImpl;
};
}
#endif
