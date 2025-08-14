#ifndef US8_MESSAGE_FORMATS_MESSAGE_HPP
#define US8_MESSAGE_FORMATS_MESSAGE_HPP
#include <memory>
#include <string_view>
namespace US8::MessageFormats
{
/// @class IMessage "message.hpp" "us8/messageFormats/message.hpp"
/// @brief An abstract base class that defines the requirements a message
///        format must satisfy.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
/// @ingroup Messages_BaseClass
class IMessage
{
public:
    /// @brief Constructor.
    //IMessage();
    /// @brief Constructs from a message stored in a string container.
    //explicit IMessage(const std::string &message);
    /// @brief Destructor.
    virtual ~IMessage();
    /// @brief Create a copy of this class.
    /// @result A copy of this class.
    [[nodiscard]] virtual std::unique_ptr<IMessage> clone() const = 0;
    /// @brief Create a clone of this class.
    [[nodiscard]] virtual std::unique_ptr<IMessage> createInstance() const noexcept = 0;
    /// @brief Converts this class to a byte-stream representation.
    /// @result The class expressed in string format.
    /// @note Though the container is a string the message need not be
    ///       human readable.
    [[nodiscard]] virtual std::string serialize() const = 0;
    /// @brief Converts this message from a byte-stream representation to a class.
    virtual void deserialize(const std::string &message);
    /// @brief Converts this message from a byte-stream representation to a class.
    virtual void deserialize(const std::string_view &message) = 0;
    /// @brief Converts this message from a byte-stream representation to a class.
    virtual void deserialize(const char *data, size_t length);
    /// @result The message type.
    [[nodiscard]] virtual std::string getMessageType() const noexcept = 0;
    /// @result The message version.
    [[nodiscard]] virtual std::string getMessageVersion() const noexcept = 0;
};
}
#endif
