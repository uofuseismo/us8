#ifndef PRIVATE_CONTROLLER_CONTROLLER_HPP
#define PRIVATE_CONTROLLER_CONTROLLER_HPP
#include <spdlog/spdlog.h>
#include <oatpp/web/server/api/ApiController.hpp>
#include <oatpp/core/macro/codegen.hpp>
#include <oatpp/core/macro/component.hpp>
#include <oatpp-websocket/Handshaker.hpp>
#include "webServer/dto/errorMessage.hpp"
#include "webServer/dto/responseMessage.hpp"

namespace US8::WebServer::Controllers
{

#include OATPP_CODEGEN_BEGIN(ApiController)

class BasicAuthorizationObject final : public oatpp::web::server::handler::AuthorizationObject
{
public:
    BasicAuthorizationObject(const oatpp::String &userName) :
       userId(userName),
       password("abc123")
    {
    }
    oatpp::String userId;
    oatpp::String password;
};

class BasicAuthorizationHandler : public oatpp::web::server::handler::BasicAuthorizationHandler
{
public:
    BasicAuthorizationHandler() :
        oatpp::web::server::handler::BasicAuthorizationHandler("us8-realm")
    {
    }
    std::shared_ptr<AuthorizationObject> authorize(
        const oatpp::String &userName, const oatpp::String &password) override
    {
        std::string user = *userName;
        if (!user.empty())
        {
            spdlog::info("Allowing " + user);
            return std::make_shared<BasicAuthorizationObject> ("uid-" + user);
        }
        return nullptr;
    } 
};

/// @brief API controller.
class MyController : public oatpp::web::server::api::ApiController {
public:
    MyController(OATPP_COMPONENT(std::shared_ptr<ObjectMapper>, objectMapper)) :
        oatpp::web::server::api::ApiController(objectMapper)
    {
        spdlog::debug("Construct");
        setDefaultAuthorizationHandler(std::make_shared<US8::WebServer::Controllers::BasicAuthorizationHandler> ());
    }
  

/*
    /// Constructor with object mapper.
    /// @param[in] objectMapper - Object mapper used to serialize/deserialize DTOs.
    Controller() :
        Controller(OATPP_COMPONENT(std::shared_ptr<ObjectMapper>, objectMapper))
    {
        //return std::make_shared<Controller> ( Controller(objectMapper)) ; 
        //setDefaultAuthorizationHandler(std::make_shared<US8::WebServer::BasicAuthorizationHandler> ());
    }
*/
/*
    Controller(std::shared_ptr<ObjectMapper> objectMapper,
               std::shared_ptr<oatpp::websocket::ConnectionHandler> webSocketConnectionHandler) :
        oatpp::web::server::api::ApiController(objectMapper),
        mWebSocketConnectionHandler(webSocketConnectionHandler)
    {
    }
*/
public:
    ENDPOINT("GET", "/", root) {

       const char* pageTemplate =
      "<html lang='en'>"
        "<head>"
          "<meta charset=utf-8/>"
        "</head>"
        "<body>"
          "<p>Hello Multithreaded WebSocket Server!</p>"
          "<p>"
            "<code>websocket endpoint is: localhost:8000/ws</code>"
          "</p>"
        "</body>"
      "</html>";

      return createResponse(Status::CODE_200, pageTemplate);
    };

    ENDPOINT("GET", "ws", ws,
             REQUEST(std::shared_ptr<IncomingRequest>, request),
             AUTHORIZATION(std::shared_ptr<oatpp::web::server::handler::DefaultBasicAuthorizationObject>,
                           authObject,
                           mBasicAuthorizationHandler)) 
    {
        std::string userName = *authObject->userId;
        OATPP_ASSERT_HTTP(!userName.empty(), // TODO
                          oatpp::web::protocol::http::Status::CODE_401,
                          "Unauthorized")
        spdlog::info("Upgrading " + userName + " to websocket connection");
        return oatpp::websocket::Handshaker::serversideHandshake(request->getHeaders(), webSocketConnectionHandler);
    };
 /* 
    ENDPOINT_ASYNC("GET", "/", root) 
    {
        ENDPOINT_ASYNC_INIT(root)
        const char* pageTemplate =
        "<html lang='en'>"
           "<head>"
              "<meta charset=utf-8/>"
           "</head>"
           "<body>"
              "<p>Hello oatpp WebSocket benchmark!</p>"
              "<p>"
                "You may connect WebSocket client on '&lt;host&gt;:&lt;port&gt;/ws'"
             "</p>"
           "</body>"
         "</html>";
         Action act() override {
         return _return(controller->createResponse(Status::CODE_200, pageTemplate));
    }
    
    };

    ENDPOINT_ASYNC("GET", "ws", ws) 
    {
        ENDPOINT_ASYNC_INIT(ws)

        Action act() override {
           auto response = oatpp::websocket::Handshaker::serversideHandshake(request->getHeaders(), controller->webSocketConnectionHandler);
           auto parameters = std::make_shared<oatpp::network::ConnectionHandler::ParameterMap>();
           response->setConnectionUpgradeParameters(parameters);
           return _return(response);
        }
     };
*/
/*

    ENDPOINT("GET", "/test", test)
    {
        auto dto = US8::WebServer::DTO::ResponseMessage::createShared();
        dto->statusCode = 200;
        dto->message = "Success";
        dto->payload = "{}";
        return createDtoResponse(Status::CODE_200, dto);
    }

    ENDPOINT("GET", "ws", ws,
             REQUEST(std::shared_ptr<IncomingRequest>, request),
             AUTHORIZATION(std::shared_ptr<oatpp::web::server::handler::DefaultBasicAuthorizationObject>, authObject, mBasicAuthorizationHandler))
    {
        //std::cout << *request->getHeader("Authorization") << std::endl; //->getHeader("basic");
        //std::cout << *authObject->userId << std::endl;
        std::string userName = *authObject->userId;
        //std::cout << *authObject->password << std::endl;
        OATPP_ASSERT_HTTP(!userName.empty(), // TODO
                          oatpp::web::protocol::http::Status::CODE_401,
                          "Unauthorized")
        spdlog::info("Upgrading " + userName + " to websocket connection");
        return oatpp::websocket::Handshaker::serversideHandshake(
            request->getHeaders(), mWebSocketConnectionHandler); 
    }
*/
   // TODO Insert Your endpoints here !!!

private:
    OATPP_COMPONENT(std::shared_ptr<oatpp::network::ConnectionHandler>, webSocketConnectionHandler, "websocket");
    //OATPP_COMPONENT(std::shared_ptr<oatpp::websocket::AsyncConnectionHandler>, webSocketConnectionHandler);
    //std::shared_ptr<oatpp::websocket::ConnectionHandler> mWebSocketConnectionHandler{nullptr};
    //std::shared_ptr<oatpp::web::server::handler::AuthorizationHandler> mBasicAuthorizationHandler = std::make_shared<oatpp::web::server::handler::BasicAuthorizationHandler>("my-realm");
    std::shared_ptr<oatpp::web::server::handler::AuthorizationHandler>
        mBasicAuthorizationHandler
    {
        std::make_shared<US8::WebServer::Controllers::BasicAuthorizationHandler> ()
    };
/*
    std::shared_ptr<oatpp::web::server::handler::AuthorizationHandler>
        mBearerAuthorizationHandler
    {
        std::make_shared<oatpp::web::server::handler::BearerAuthorizationHandler>("my-realm")
    };
*/
};

#include OATPP_CODEGEN_END(ApiController)


}

#endif
