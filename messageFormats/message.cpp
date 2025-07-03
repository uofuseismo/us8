#include "us8/messageFormats/message.hpp"

using namespace US8::MessageFormats;

/// Constructor
IMessage::IMessage() = default;

/// Constructor
IMessage::IMessage(const std::string &message)
{
    deserialize(message);
}

/// Destructor
IMessage::~IMessage() = default;

/// Constructor
void IMessage::deserialize(const std::string &message)
{
    deserialize(message.c_str(), message.size());
}
