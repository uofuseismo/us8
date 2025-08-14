#ifndef US8_SERVICES_WEB_SERVER_EXCEPTION_HPP
#define US8_SERVICES_WEB_SERVER_EXCEPTION_HPP
#include <string>
#include <exception>
namespace US8::Services::WebServer::Exception
{
/// @brief This should result in a 403 FORBIDDEN error.
/// @copyright Ben Baker (UUSS) distributed under the MIT license.
class InvalidPermission final : public std::exception 
{
public:
    InvalidPermission(const std::string &message);
    InvalidPermission(const char *message);
    ~InvalidPermission() final;
    const char *what() const noexcept override final;
private:
    std::string mMessage;
};
/// @brief This should result in a 400 Bad Request error.
/// @copyright Ben Baker (UUSS) distributed under the MIT license.
class BadRequest final : public std::exception
{
public:
    BadRequest(const std::string &message);
    BadRequest(const char *message);
    ~BadRequest() final;
    const char *what() const noexcept override final;
private:
    std::string mMessage;
};
/// @brief This should result in a 501 not-implemented error.
/// @copyright Ben Baker (UUSS) distributed under the MIT license.
class Unimplemented final : public std::exception 
{
public:
    Unimplemented(const std::string &message);
    Unimplemented(const char *message);
    ~Unimplemented() final;
    const char *what() const noexcept override final;
private:
    std::string mMessage;
};
/// @brief This should result in a 204 no-content error.
/// @copyright Ben Baker (UUSS) distributed under the MIT license.
class NoContent final : public std::exception 
{
public:
    NoContent(const std::string &message);
    NoContent(const char *message);
    ~NoContent() final;
    const char *what() const noexcept override final;
private:
    std::string mMessage;
};
/// @brief This should result in a 404 not found error.
/// @copyright Ben Baker (UUSS) distributed under the MIT license.
class NotFound final : public std::exception 
{
public:
    NotFound(const std::string &message);
    NotFound(const char *message);
    ~NotFound() final;
    const char *what() const noexcept override final;
private:
    std::string mMessage;
};
}
#endif
