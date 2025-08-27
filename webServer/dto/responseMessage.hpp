#ifndef US8_WEB_SERVER_RESPONSE_MESSAGE_DTO_HPP
#define US8_WEB_SERVER_RESPONSE_MESSAGE_DTO_HPP
#include <oatpp/core/data/mapping/type/Object.hpp>
#include <oatpp/core/macro/codegen.hpp>

namespace US8::WebServer::DTO
{

#include OATPP_CODEGEN_BEGIN(DTO)

class ResponseMessage : public oatpp::DTO {

  DTO_INIT(ResponseMessage, DTO);     // Extends 
  DTO_FIELD(Int32,  statusCode);    // Status code field
  DTO_FIELD(String, message); // Response message
  DTO_FIELD(String, payload);      // Message field

};

#include OATPP_CODEGEN_END(DTO)

}
#endif
