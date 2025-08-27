#ifndef PRIVATE_WEB_SERVER_WEB_SOCKET_INSTANCE_LISTENER_HPP
#define PRIVATE_WEB_SERVER_WEB_SOCKET_INSTANCE_LISTENER_HPP
#include <oatpp-websocket/ConnectionHandler.hpp>
#include <oatpp-websocket/WebSocket.hpp>
#include <oatpp/parser/json/mapping/ObjectMapper.hpp>
#include "listener.hpp"

#include <atomic>

namespace US8::WebServer::WebSocket
{

class InstanceListener final : public oatpp::websocket::ConnectionHandler::SocketInstanceListener
{
private:
    static constexpr std::string_view mTag{"ServerWebSocketInstanceListener"}; 
    std::shared_ptr<oatpp::parser::json::mapping::ObjectMapper> mObjectMapper{nullptr};
public:
    /// @brief Counter for connected clients.
    std::atomic<int32_t> mSocketCounter{0};
public:
    explicit InstanceListener( std::shared_ptr<oatpp::parser::json::mapping::ObjectMapper> objectMapper) :
        mObjectMapper(objectMapper)
    {
    }
    void onAfterCreate(const oatpp::websocket::WebSocket &socket,
                       const std::shared_ptr<const ParameterMap> &params) override
    {   
        ++mSocketCounter;
        spdlog::info("ServerWebSocketInstanceListener receiving incoming connection.  Number of connections is "
                   + std::to_string(mSocketCounter.load()));
        // Here we're creating one listener per connection.
        // However, this may be redundant.
        socket.setListener(std::make_shared<US8::WebServer::WebSocket::Listener> (mObjectMapper));
    }   
    void onBeforeDestroy(const oatpp::websocket::WebSocket &socket) override
    {   
        --mSocketCounter;
        spdlog::info("ServerWebSocketInstanceListener closing connection.  Number of connections is "
                   + std::to_string(mSocketCounter.load()));
    }   
};

}
#endif
