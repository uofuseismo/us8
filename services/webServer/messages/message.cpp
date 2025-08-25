#include <string>
#include <nlohmann/json.hpp>
#include "us8/services/webServer/messages/message.hpp"

using namespace US8::Services::WebServer::Messages;

/// Destructor
IMessage::~IMessage() = default;

/// Status code - assume 200
int IMessage::getStatusCode() const noexcept
{
    return 200;
}

/// Success - assume okay
bool IMessage::getSuccess() const noexcept
{
    return true;
}
 
/// Data
std::optional<nlohmann::json> IMessage::getData() const noexcept
{
    return std::nullopt;
}

/// Message?
std::optional<std::string> IMessage::getMessage() const noexcept
{
    return std::nullopt;
}

/// Serialize the message
std::string US8::Services::WebServer::Messages::toJSON(
    const IMessage &message,
    const int indentIn)
{
    const int indent = indentIn >= 0 ? indentIn : -1;
    std::string result;
    nlohmann::json object;
/*
    if (message)
    {
*/
        object["success"] = message.getSuccess();
        object["statusCode"] = message.getStatusCode();
        auto messageDetails = message.getMessage();
        if (messageDetails)
        {
            object["message"] = std::move(*messageDetails);
        }
        else
        {
            object["message"] = nullptr;
        }
        auto data = message.getData();
        if (data)
        {
            object["data"] = std::move(*data);
        }
        else
        {
            object["data"] = nullptr;
        }
 /*
    }
    else
    {
        object["success"] = false;
        object["statusCode"] = 500;
        object["message"] = "server created null response message";
        object["data"] = nullptr;
    }
*/
    return object.dump(indent);
}
