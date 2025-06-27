#include "us8/messageFormats/message.hpp"

using namespace US8::MessageFormats;

/// Constructor
IMessage::IMessage() = default;

/// Constructor
IMessage::IMessage(const std::string &message)
{
    fromMessage(message);
}

/// Destructor
IMessage::~IMessage() = default;

/// Constructor
void IMessage::fromMessage(const std::string &message)
{
    fromMessage(message.c_str(), message.size());
}
