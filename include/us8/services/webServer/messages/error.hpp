#ifndef US8_SERVICES_WEB_SERVER_MESSAGES_ERROR_HPP
#define US8_SERVICES_WEB_SERVER_MESSAGES_ERROR_HPP
#include <memory>
#include <optional>
#include <us8/services/webServer/messages/message.hpp>
namespace US8::Services::WebServer::Messages
{
/// @class Error "error.hpp" "mlReview/messages/error.hpp"
/// @brief Defines a reusable error message to return to the client.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class Error final : public IMessage
{
public:
    /// @brief Constructor.
    Error();
    /// @brief Copy constructor.
    Error(const Error &error);
    /// @brief Move constructor.
    Error(Error &&error) noexcept;

    /// @brief Sets an HTTP status code associated with the request.
    void setStatusCode(int code) noexcept;
    /// @result The error code.
    [[nodiscard]] int getStatusCode() const noexcept override final;

    /// @brief Sets the details of the error message. 
    /// @param[in] details  The details of the error message. 
    void setMessage(const std::string &details) noexcept;
    /// @result The details of the error message.
    [[nodiscard]] std::optional<std::string> getMessage() const noexcept final;

    /// @result Flag indicating the request was successful.
    [[nodiscard]] bool getSuccess() const noexcept override final;

    /// @brief Destructor.
    ~Error() override;

    /// @brief Clones this class.
    [[nodiscard]] std::unique_ptr<IMessage> clone() const;

    /// @brief Copy assignment.
    Error& operator=(const Error &error);
    /// @brief Move assignment.
    Error& operator=(Error &&error) noexcept;
private:
    class ErrorImpl;
    std::unique_ptr<ErrorImpl> pImpl;
};
}
#endif

