#include <string>
#include <spdlog/spdlog.h>
#include <oatpp-websocket/WebSocket.hpp>
#include "listener.hpp"
#include "webServer/dto/errorMessage.hpp"
#include "webServer/dto/responseMessage.hpp"

using namespace US8::WebServer::WebSocket;

void Listener::onPing(const oatpp::websocket::WebSocket &socket,
                      const oatpp::String &message)
{
    spdlog::debug("ServerWebSocketListener ping");
    socket.sendPong(message);
}

void Listener::onPong(const oatpp::websocket::WebSocket &socket,
                      const oatpp::String &message)
{
    spdlog::debug("ServerWebSocketListener pong");
}

void Listener::onClose(const oatpp::websocket::WebSocket &socket,
                       const uint16_t code, 
                       const oatpp::String &)
{
    //OATPP_LOGD(mTag, " onClose={}", code);
    spdlog::debug(
        "ServerWebSocketListener: Closing web socket with code "
      + std::to_string(code));
}

/*
void Listener::sendMessage(const oatpp::websocket::WebSocket &socket,
                           const US8 
*/

void Listener::readMessage(const oatpp::websocket::WebSocket &socket,
                           const uint8_t operationCode,
                           p_char8 data,
                           oatpp::v_io_size size)
{   
    // Message transfer finished
    if (size == 0)
    {
        auto entireMessage = mMessageBuffer.toString();
        spdlog::info("Request: " + *entireMessage);
        mMessageBuffer.setCurrentPosition(0);
        std::string replyMessage;
        try
        {
            auto responseMessage
                = US8::WebServer::DTO::ResponseMessage::createShared();
            responseMessage->statusCode = 200;
            responseMessage->message = "Success";
            responseMessage->payload = entireMessage;
            auto responseMessageString = mObjectMapper->writeToString(responseMessage);
            // Send message in reply
            socket.sendOneFrameText(responseMessageString); //"Replying from success oat");
        }
        catch (const std::exception &e) 
        {
            auto errorMessage = US8::WebServer::DTO::ErrorMessage::createShared();
            errorMessage->statusCode = 400;
            errorMessage->reason = "Server error";
            auto errorMessageString = mObjectMapper->writeToString(errorMessage);
            socket.sendOneFrameText("stuff");//errorMessage); //"Replying from oat websocket");
        }
    }
    else if (size > 0)
    {
        mMessageBuffer.writeSimple(data, size);
    }
    else
    {
        spdlog::warn("Negative size in WebSocket::Listener::readMessage");
    }
}
