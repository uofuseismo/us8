#ifndef PRIVATE_WEB_SERVER_WEB_SOCKET_LISTENER_HPP
#define PRIVATE_WEB_SERVER_WEB_SOCKET_LISTENER_HPP
#include <oatpp-websocket/WebSocket.hpp>
#include <oatpp/parser/json/mapping/ObjectMapper.hpp>
namespace US8::WebServer::WebSocket
{
class Listener final : public oatpp::websocket::WebSocket::Listener 
{
public:
    explicit Listener(std::shared_ptr<oatpp::parser::json::mapping::ObjectMapper> &objectMapper) :
        mObjectMapper(objectMapper)
    {
    }
    /// @brief Respond to a ping message.
    void onPing(const oatpp::websocket::WebSocket &socket,
                const oatpp::String &message) override;
    /// @brief Issue a pong message.
    void onPong(const oatpp::websocket::WebSocket &socket,
                const oatpp::String &message) override;
    /// @brief Called on the "close" frame.
    void onClose(const oatpp::websocket::WebSocket &socket,
                 const uint16_t code, 
                 const oatpp::String &) override;
    /// @brief Called on each frame.  After, the last message will be called
    ///        once-again with size == 0 to designate the end of the message.
    void readMessage(const oatpp::websocket::WebSocket &socket,
                     const uint8_t operationCode,
                     p_char8 data,
                     oatpp::v_io_size size) override;
private:
    oatpp::data::stream::BufferOutputStream mMessageBuffer;
    std::shared_ptr<oatpp::parser::json::mapping::ObjectMapper> mObjectMapper{nullptr};
};
}
#endif
