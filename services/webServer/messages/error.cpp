#include <nlohmann/json.hpp>
#include "us8/services/webServer/messages/error.hpp"

using namespace US8::Services::WebServer::Messages;

class Error::ErrorImpl
{
public:
    std::string mMessage;
    int mStatusCode{400};
    bool mSuccess{false};
};

/// Constructor
Error::Error() :
    pImpl(std::make_unique<ErrorImpl> ())
{
}

/// Copy constructor
Error::Error(const Error &error)
{
    *this = error;
}

/// Move constructor
Error::Error(Error &&error) noexcept
{
    *this = std::move(error);
}

/// Copy assignment
Error& Error::operator=(const Error &error)
{
    if (&error == this){return *this;}
    pImpl = std::make_unique<ErrorImpl> (*error.pImpl);
    return *this;
}

/// Move assignment
Error& Error::operator=(Error &&error) noexcept
{
    if (&error == this){return *this;}
    pImpl = std::move(error.pImpl);
    return *this;
}

/// Destructor
Error::~Error() = default;

/// Error details
void Error::setMessage(const std::string &details) noexcept
{
    pImpl->mMessage = details;
}

std::optional<std::string> Error::getMessage() const noexcept
{
    return !pImpl->mMessage.empty() ?
           std::optional<std::string> (pImpl->mMessage) : std::nullopt;
}

/// Copy this class
std::unique_ptr<US8::Services::WebServer::Messages::IMessage> Error::clone() const
{
    std::unique_ptr<Messages::IMessage> result
        = std::make_unique<Error> (*this);
    return result;
}

/// Error code
void Error::setStatusCode(const int code) noexcept
{
    pImpl->mStatusCode = code;
}

int Error::getStatusCode() const noexcept
{
    return pImpl->mStatusCode;
}

/// Success?
bool Error::getSuccess() const noexcept
{
    return pImpl->mSuccess;
}

