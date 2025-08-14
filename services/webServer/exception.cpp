#include <string>
#include <exception>
#include "us8/services/webServer/exception.hpp"

using namespace US8::Services::WebServer::Exception;

// Invalid permission
InvalidPermission::InvalidPermission(const std::string &message) :
    mMessage(message)
{
}

InvalidPermission::InvalidPermission(const char *message) :
    mMessage(message)
{
}

InvalidPermission::~InvalidPermission() = default;

const char *InvalidPermission::what() const noexcept
{
    return mMessage.c_str();
}

// Bad request
BadRequest::BadRequest(const std::string &message) :
    mMessage(message)
{   
}

BadRequest::BadRequest(const char *message) :
    mMessage(message)
{   
}

BadRequest::~BadRequest() = default;

const char *BadRequest::what() const noexcept
{   
   return mMessage.c_str();
}   

// Unimplemented
Unimplemented::Unimplemented(const std::string &message) :
    mMessage(message)
{   
}   

Unimplemented::Unimplemented(const char *message) :
    mMessage(message)
{
}

Unimplemented::~Unimplemented() = default;

const char *Unimplemented::what() const noexcept
{   
    return mMessage.c_str();
}   
 
// No content
NoContent::NoContent(const std::string &message) :
    mMessage(message)
{
}   

NoContent::NoContent(const char *message) :
   mMessage(message)
{   
}

NoContent::~NoContent() = default;

const char *NoContent::what() const noexcept
{
    return mMessage.c_str();
}
    
// Not found
NotFound::NotFound(const std::string &message) :
    mMessage(message)
{   
}   

NotFound::NotFound(const char *message) :
    mMessage(message)
{
}

NotFound::~NotFound() = default;

const char *NotFound::what() const noexcept
{
   return mMessage.c_str();
}   

