#include <string>
#include <string_view>
#include "us8/messageFormats/message.hpp"

using namespace US8::MessageFormats;

/// Destructor
IMessage::~IMessage() = default;

/// Constructor
void IMessage::deserialize(const std::string &message)
{
    deserialize(std::string_view {message});
}

void IMessage::deserialize(const char *data, const size_t length)
{
    if (length == 0){throw std::invalid_argument("No data");}
    if (data == nullptr)
    {   
        throw std::invalid_argument("message data is null");
    }   
    const std::string_view messageStringView{data, length};
    return deserialize(messageStringView);
}   
