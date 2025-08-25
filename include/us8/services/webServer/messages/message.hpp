#ifndef US8_SERVICES_WEB_SERVER_MESSAGES_MESSAGE_HPP
#define US8_SERVICES_WEB_SERVER_MESSAGES_MESSAGE_HPP
#include <memory>
#include <string>
#include <optional>
#include <nlohmann/json.hpp>
namespace US8::Services::WebServer::Messages
{
class IMessage
{
public:
    /// @brief Destructor.
    virtual ~IMessage();
    /// @result The status code associated with this request.
    [[nodiscard]] virtual int getStatusCode() const noexcept;
    /// @result True indicates the API call was a success.
    [[nodiscard]] virtual bool getSuccess() const noexcept;
    /// @result True a message accompanying the response.
    ///         By defualt this is null.
    [[nodiscard]] virtual std::optional<std::string> getMessage() const noexcept;
    /// @result The data associated with this request.
    ///         By default this is null.
    [[nodiscard]] virtual std::optional<nlohmann::json> getData() const noexcept; 
};
[[nodiscard]] std::string toJSON(const IMessage &message, const int indent =-1);
}
#endif
