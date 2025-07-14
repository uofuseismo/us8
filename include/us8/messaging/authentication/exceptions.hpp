#ifndef US8_MESSAGING_AUTHENTICATION_EXCEPTIONS_HPP
#define US8_MESSAGING_AUTHENTICATION_EXCEPTIONS_HPP
#include <exception>
#include <string>
namespace US8::Messaging::Authentication::Exceptions
{
/// @brief This should result in a 400 Bad Request error.
///        This indicates that the credentials are malformed.
/// @copyright Ben Baker (UUSS) distributed under the MIT license.
class BadRequest final : public std::exception
{
public:
    BadRequest(const std::string &message) :
        mMessage(message)
    {   
    }   
    BadRequest(const char *message) :
        mMessage(message)
    {   
    }   
    ~BadRequest() final = default;
    virtual const char *what () const noexcept final
    {   
        return mMessage.c_str();
    }   
private:
    std::string mMessage;
};

/// @brief This should result in a 401 Unauthorized error.
///        This indicates that the credentials are not valid.
/// @copyright Ben Baker (UUSS) distributed under the MIT license.
class Unauthorized final : public std::exception
{
public:
    Unauthorized(const std::string &message) :
        mMessage(message)
    {   
    }   
    Unauthorized(const char *message) :
        mMessage(message)
    {   
    }   
    ~Unauthorized() final = default;
    virtual const char *what () const noexcept final
    {   
        return mMessage.c_str();
    }   
private:
    std::string mMessage;
};

/// @brief This should result in a 403 Forbidden error.
///        In this case, the credentials are valid but the privileges
///        are insufficient.
/// @copyright Ben Baker (UUSS) distributed under the MIT license.
class Forbidden final : public std::exception
{
public:
    Forbidden(const std::string &message) :
        mMessage(message)
    { 
    }
    Forbidden(const char *message) :
        mMessage(message)
    {   
    }   
    ~Forbidden() final = default;
    virtual const char *what () const noexcept final
    {
        return mMessage.c_str();
    }
private:
    std::string mMessage;
};

/// @brief This should result in a 500 Internal Server error.
/// @copyright Ben Baker (UUSS) distributed under the MIT license.
class InternalServerError final : public std::exception
{
public:
    InternalServerError(const std::string &message) :
        mMessage(message)
    {   
    }   
    InternalServerError(const char *message) :
        mMessage(message)
    {   
    }   
    ~InternalServerError() final = default;
    virtual const char *what () const noexcept final
    {   
        return mMessage.c_str();
    }   
private:
    std::string mMessage;
};

}
#endif
