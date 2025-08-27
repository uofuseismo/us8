#ifndef PRIVATE_CONTROLLER_CONTROLLER_HPP
#define PRIVATE_CONTROLLER_CONTROLLER_HPP
#include <spdlog/spdlog.h>
#include <oatpp/web/server/api/ApiController.hpp>
#include <oatpp/core/macro/codegen.hpp>
#include <oatpp/core/macro/component.hpp>
#include <oatpp-websocket/Handshaker.hpp>
#include "webServer/dto/errorMessage.hpp"
#include "webServer/dto/responseMessage.hpp"

namespace US8::WebServer
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
class Controller : public oatpp::web::server::api::ApiController {
public:
    /// Constructor with object mapper.
    /// @param[in] objectMapper - Object mapper used to serialize/deserialize DTOs.
    explicit Controller(std::shared_ptr<ObjectMapper> objectMapper) : //OATPP_COMPONENT(std::shared_ptr<ObjectMapper>, objectMapper))
        oatpp::web::server::api::ApiController(objectMapper)
    {
        setDefaultAuthorizationHandler(std::make_shared<US8::WebServer::BasicAuthorizationHandler> ());
    }
    Controller(std::shared_ptr<ObjectMapper> objectMapper,
               std::shared_ptr<oatpp::websocket::ConnectionHandler> webSocketConnectionHandler) :
        oatpp::web::server::api::ApiController(objectMapper),
        mWebSocketConnectionHandler(webSocketConnectionHandler)
    {
    }
public:
  
    ENDPOINT("GET", "/test", root)
    {
        auto dto = US8::WebServer::DTO::ResponseMessage::createShared();
        dto->statusCode = 200;
        dto->message = "Success";
        dto->payload = "{}";
        return createDtoResponse(Status::CODE_200, dto);
    }

    ENDPOINT("GET", "ws", ws,
             REQUEST(std::shared_ptr<IncomingRequest>, request),
             AUTHORIZATION(std::shared_ptr<oatpp::web::server::handler::DefaultBasicAuthorizationObject>, authObject, mBasicAuthHandler))
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
   // TODO Insert Your endpoints here !!!

private:
    std::shared_ptr<oatpp::websocket::ConnectionHandler> mWebSocketConnectionHandler{nullptr};
    //std::shared_ptr<oatpp::web::server::handler::AuthorizationHandler> mBasicAuthHandler = std::make_shared<oatpp::web::server::handler::BasicAuthorizationHandler>("my-realm");
    std::shared_ptr<oatpp::web::server::handler::AuthorizationHandler>
        mBasicAuthHandler
    {
        std::make_shared<US8::WebServer::BasicAuthorizationHandler> ()
    };
//TODO
    std::shared_ptr<oatpp::web::server::handler::AuthorizationHandler>
        mBearerAuthHandler
    {
        std::make_shared<oatpp::web::server::handler::BearerAuthorizationHandler>("my-realm")
    };
};

#include OATPP_CODEGEN_END(ApiController)

}

#endif
