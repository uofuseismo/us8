#include <iostream>
#include <atomic>
#include <memory>
#include <spdlog/spdlog.h>
#include <oatpp-websocket/ConnectionHandler.hpp>
#include <oatpp/web/server/HttpConnectionHandler.hpp>
#include <oatpp/web/server/HttpRouter.hpp>
#include <oatpp/network/tcp/server/ConnectionProvider.hpp>
#include <oatpp/network/Server.hpp>
#include <oatpp/core/macro/component.hpp>
#include <oatpp/core/base/Environment.hpp>
#include <oatpp/parser/json/mapping/ObjectMapper.hpp>

#include "oatpp/web/server/api/ApiController.hpp"

/*
#include <oatpp-websocket/ConnectionHandler.hpp>
#include <oatpp-websocket/WebSocket.hpp>
*/

#include <oatpp/core/macro/codegen.hpp>

#include "webSocket/listener.hpp"
#include "webSocket/instanceListener.hpp"
#include "dto/responseMessage.hpp"
#include "dto/errorMessage.hpp"

#include "controller/controller.hpp"

namespace
{

/*
#include OATPP_CODEGEN_BEGIN(DTO)
class MessageDto : public oatpp::DTO {

  DTO_INIT(MessageDto, DTO); 

  DTO_FIELD(Int32,  statusCode);  // Status code field
  DTO_FIELD(String, payload);     // Payload
  DTO_FIELD(String, message);     // Message field
};

class ErrorMessageDto : public oatpp:: DTO
{
    DTO_INIT(ErrorMessageDto, DTO); /// Extends
    DTO_FIELD(Int32, statusCode); /// Error code - e.g., 404
    DTO_FIELD(String, reason);    /// Reason for failure.
};

#include OATPP_CODEGEN_END(DTO)

#include OATPP_CODEGEN_BEGIN(ApiController)
class MyController final : public oatpp::web::server::api::ApiController
{
public:
    MyController(OATPP_COMPONENT(std::shared_ptr<oatpp::data::mapping::ObjectMapper>, objectMapper))
        : oatpp::web::server::api::ApiController(objectMapper)
    {
    }
public:
    ENDPOINT("GET", "/hello", root)
    {
        auto responseMessage = MessageDto::createShared();
        responseMessage->statusCode = 200;
        responseMessage->payload = "{}";
        responseMessage->message = "Hello dto controller";
        return createDtoResponse(Status::CODE_200, responseMessage);
    }
};
#include OATPP_CODEGEN_END(ApiController)
 
class Handler : public oatpp::web::server::HttpRequestHandler
{
private:
    std::shared_ptr<oatpp::data::mapping::ObjectMapper> mObjectMapper;
public:
    /// @brief Constructor with object mapper.
    explicit Handler(const std::shared_ptr<oatpp::data::mapping::ObjectMapper> &objectMapper) :
        mObjectMapper(objectMapper)
    {
    }
    /// @brief Handles an incoming request and returns an outgoing message.
    std::shared_ptr<OutgoingResponse> handle(const std::shared_ptr<IncomingRequest>& request) override
    {
        auto message = ::MessageDto::createShared();
        message->statusCode = 200;
        message->payload = "{}";
        message->message = "Hello DTO!";
        return ResponseFactory::createResponse(Status::CODE_200, message, mObjectMapper);
    }
};

*/

/*
class WebSocketListener final : public oatpp::websocket::WebSocket::Listener 
{
private:
    static constexpr std::string_view mTag{"ServerWebSocketListener"};
public:
    void onPing(const oatpp::websocket::WebSocket &socket,
                const oatpp::String &message) override
    {
        spdlog::debug("ServerWebSocketListener ping");
        socket.sendPong(message);
    }; 
    void onPong(const oatpp::websocket::WebSocket &socket,
                const oatpp::String &message) override
    {
        spdlog::debug("ServerWebSocketListener pong");
    };
    /// @brief Called on the "close" frame.
    void onClose(const oatpp::websocket::WebSocket &socket,
                 const uint16_t code, 
                 const oatpp::String &) override
    {
        //OATPP_LOGD(mTag, " onClose={}", code);
        spdlog::debug("ServerWebSocketListener: Closing web socket with code ");
    }
    /// @brief Called on each frame.  After, the last message will be called
    ///        once-again with size == 0 to designate the end of the message.
    void readMessage(const oatpp::websocket::WebSocket &socket,
                     const uint8_t operationCode,
                     p_char8 data,
                     oatpp::v_io_size size) override
    {
        // Message transfer finished
        if (size == 0)
        {
            auto entireMessage = mMessageBuffer.toString();
            mMessageBuffer.setCurrentPosition(0);
            std::string replyMessage;
            try
            {
                // Send message in reply
                socket.sendOneFrameText("Replying from success oat");
            }
            catch (const std::exception &e)
            {
                socket.sendOneFrameText("Replying from oat websocket");
            } 
        }
        else if (size > 0)
        {
            mMessageBuffer.writeSimple(data, size);
        }
    }
private:
    oatpp::data::stream::BufferOutputStream mMessageBuffer;
};
*/

/*
class WebSocketInstanceListener final : public oatpp::websocket::ConnectionHandler::SocketInstanceListener
{
private:
    static constexpr std::string_view mTag{"ServerWebSocketInstanceListener"}; 
public:
    /// @brief Counter for connected clients.
    std::atomic<int32_t> mSocketCounter{0};
public:
    void onAfterCreate(const oatpp::websocket::WebSocket &socket,
                       const std::shared_ptr<const ParameterMap> &params) override
    {
        ++mSocketCounter;
        spdlog::info("ServerWebSocketInstanceListener receiving incoming connection.  Number of connections is "
                   + std::to_string(mSocketCounter.load()));
        // Here we're creating one listener per connection.
        // However, this may be redundant.
        socket.setListener(std::make_shared<US8::WebServer::WebSocket::Listener> ()); 
    }
    void onBeforeDestroy(const oatpp::websocket::WebSocket &socket) override
    {
        --mSocketCounter;
        spdlog::info("ServerWebSocketInstanceListener closing connection.  Number of connections is "
                   + std::to_string(mSocketCounter.load()));
    }
    
};
*/

/*
class Controller final : public oatpp::web::server::api::ApiController
{
public:
    Controller(const std::shared_ptr<oatpp::web::mime::ContentMappers> &apiContentMapers)
};
*/

class ApplicationComponents
{
public:
    ApplicationComponents(const std::string &host = "0.0.0.0",
                          uint16_t port = 80,
                          const bool useIPV4 = true) :
        mAddress{ oatpp::String {host},
                  port,
                  useIPV4 ? oatpp::network::Address::Family::IP_4 : oatpp::network::Address::Family::IP_6 }
    {
        spdlog::info("Creating listener at "
                   + *mAddress.host + ":" + std::to_string(mAddress.port));

        // Create a JSON object mapper
        mObjectMapper = oatpp::parser::json::mapping::ObjectMapper::createShared();

        serverConnectionProvider
            = oatpp::network::tcp::server::ConnectionProvider::createShared(
                 mAddress);
        httpRouter
            = oatpp::web::server::HttpRouter::createShared();
        httpConnectionHandler
            = oatpp::web::server::HttpConnectionHandler::createShared(httpRouter);

        mWebSocketConnectionHandler
            = oatpp::websocket::ConnectionHandler::createShared();
        mWebSocketConnectionHandler->setSocketInstanceListener(
            std::make_shared<US8::WebServer::WebSocket::InstanceListener>
               (mObjectMapper));
    }

    /// Create the connection provider which listens on the port
/*
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::network::ServerConnectionProvider>, serverConnectionProvider)(
       [](oatpp::network::Address address) {
       
std::cout << address.port << std::endl;
       return oatpp::network::tcp::server::ConnectionProvider::createShared(address);
    }(mAddress));
*/
/*
    /// Create the router
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>, httpRouter)([] {
       return oatpp::web::server::HttpRouter::createShared();
    }());
    /// Create the http connection handler
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::network::ConnectionHandler>, httpConnectionHandler)(
        "http", [] {
            OATPP_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>, router); // Get router
            return oatpp::web::server::HttpConnectionHandler::createShared(router);
        }());
    /// Create the websocket connection handler
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::network::ConnectionHandler>, webSocketConnectionHandler)(
        "websocket", [] {
            auto connectionHandler = oatpp::websocket::ConnectionHandler::createShared();
            connectionHandler->setSocketInstanceListener(std::make_shared<::WebSocketInstanceListener> ());
            return connectionHandler;
        }());
    ///Create the ObjectMapper to (de)serialize DTOs in the controller's API
*/
/*
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::data::mapping::ObjectMapper>, apiObjectMapper)( [] {
        return std::make_shared<oatpp::json::ObjectMapper> ();
    }());
*/
/*
    std::string mHost{"0.0.0.0"};
    uint16_t mPort{9030};
    bool mUseIPV4{true};
    oatpp::network::Address mAddress{oatpp::String {mHost},
                                     mPort,
                                     mUseIPV4 ? oatpp::network::Address::Family::IP_4 : oatpp::network::Address::Family::IP_6};
*/
    /// Address on which to bind
    oatpp::network::Address mAddress{"0.0.0.0",
                                     80,
                                     oatpp::network::Address::Family::IP_4};
    /// Binds to a specified port and provides TCP connections.
    std::shared_ptr<oatpp::network::ServerConnectionProvider>
        serverConnectionProvider{nullptr};
    /// Maps URLs to end point handlers.  If an endpoint is not found a 404
    /// error is returned.   
    std::shared_ptr<oatpp::web::server::HttpRouter>
        httpRouter{nullptr};
    /// HTTP connection handler.  This is multi-threaded and will create one
    /// thread per connection.
    std::shared_ptr<oatpp::network::ConnectionHandler>
        httpConnectionHandler{nullptr};
    /// Websocket connection handler.
    std::shared_ptr<oatpp::websocket::ConnectionHandler>
        mWebSocketConnectionHandler{nullptr};
    /// Object mapper
    std::shared_ptr<oatpp::parser::json::mapping::ObjectMapper>
        mObjectMapper{nullptr};
};

void run() 
{
std::string host = "0.0.0.0";
uint16_t port{9030};
    ::ApplicationComponents components(host, port);

    auto router = components.httpRouter;
    auto myController
        = std::make_shared<US8::WebServer::Controller>
             (components.mObjectMapper,
              components.mWebSocketConnectionHandler);

    router->addController(myController);

    //router->route("GET", "/hello", std::make_shared<::Handler> (components.mObjectMapper));
//auto myController = std::make_shared<::MyController> (components.mObjectMapper);
//    router->addController(myController); //route("GET", "/hello", std::make_shared<::Handler> (components.mObjectMapper));
/*
    // Get router
    OATPP_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>, router);

    // Create controller and add all of its endpoints to the router

    // Get connection handler component
    OATPP_COMPONENT(std::shared_ptr<oatpp::network::ConnectionHandler>, connectionHandler, "http");
*/
    auto connectionHandler = components.httpConnectionHandler;

    // Get connection provider component
    //OATPP_COMPONENT(std::shared_ptr<oatpp::network::ServerConnectionProvider>, connectionProvider);
    auto connectionProvider = components.serverConnectionProvider;
    // Create server which takes provided TCP connections and passes
    /// them to HTTP connection handler
    oatpp::network::Server server(connectionProvider, connectionHandler);

    // Print info about server port

    server.run();
}

}

int main(int argc, char *argv[])
{
    oatpp::base::Environment::init(); 

    try
    {
        run();
        oatpp::base::Environment::destroy();
    }
    catch (const std::exception &e)
    {
        spdlog::critical("Error detected while running web server.  Details "
                       + std::string {e.what()});
        oatpp::base::Environment::destroy();
        return EXIT_FAILURE;
    }       
    return EXIT_SUCCESS;
}
