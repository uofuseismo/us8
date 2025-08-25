#ifndef US8_SERVICES_WEB_SERVER_CALLBACK_HANDLER_HPP
#define US8_SERVICES_WEB_SERVER_CALLBACK_HANDLER_HPP
#include <optional>
#include <memory>
#include <set>
#include <us8/services/webServer/messages/message.hpp>
#include <us8/services/webServer/exception.hpp>
namespace US8::Services::WebServer
{

class CallbackHandler
{
public:
    /// @brief Constructor.
    CallbackHandler();
    /// @brief Move constructor.
    /// @param[in,out] handler  The handler from which to initialize this class.
    ///                         On exit, handler's behavior is undefined.
    CallbackHandler(CallbackHandler &&handler) noexcept;

    /// @brief Process the payload message.
    /// @param[in] payload  The payload to process.
    /// @throws Should an error be encountered, throws an appropriate exception
    ///         as defined the webserver exception header.   
    [[nodiscard]] std::optional<US8::Services::WebServer::Messages::IMessage> process(const std::string &payload);

    /// @brief Inserts a resource to the handler.
    //void insert(std::unique_ptr<IResource > &&resource);

    [[nodiscard]] std::set<std::string> getResources() const noexcept;

    /// @brief Destructor.
    ~CallbackHandler();

    /// @brief Move assignment operator.
    /// @param[in,out] handler  The handler whose memory will be moved to this.
    ///                         On exit, handler's behavior is undefined.
    /// @result The memory from handler moved to this.
    CallbackHandler& operator=(CallbackHandler &&handler) noexcept;

    CallbackHandler(const CallbackHandler &) = delete;
    CallbackHandler& operator=(CallbackHandler &) = delete;
private:
    class CallbackHandlerImpl;
    std::unique_ptr<CallbackHandlerImpl> pImpl;
};

}
#endif
