#ifndef SERVER_HPP
#define SERVER_HPP
// This is a customized version of Vinnie Falco's HTTP plain/SSL server
// that performs asynchronous communication.  That file was distributed
// under the Boost Software License, Version 1.0.
// (https://www.boost.org/LICENSE_1_0.txt)
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>
#include <boost/algorithm/string.hpp>
#include <functional>
#include <map>
#include <queue>
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include "us8/services/webServer/callbackHandler.hpp"
#include "us8/services/webServer/messages/error.hpp"
#include "us8/services/webServer/exception.hpp"

/*
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
*/

namespace
{

// Return a reasonable mime type based on the extension of a file.
boost::beast::string_view
getMIMEType(const boost::beast::string_view path)
{
    const std::map<std::string, std::string_view> fileMap
    {
        std::pair {".htm",     "text/html"},
        std::pair {".html",    "text/html"},
        std::pair {".php",     "text/html"},
        std::pair {".css",     "text/css"},
        std::pair {".txt",     "text/plain"},
        std::pair {".js",      "application/javascript"},
        std::pair {".json",    "application/json"},
        std::pair {".geojson", "application/json"},
        std::pair {".xml",     "application/xml"},
        std::pair {".swf",     "application/x-shockwave-flash"},
        std::pair {".flv",     "video/x-flv"},
        std::pair {".png",     "image/png"},
        std::pair {".jpe",     "image/jpeg"},
        std::pair {".jpeg",    "image/jpeg"},
        std::pair {".jpg",     "image/jpeg"},
        std::pair {".gif",     "image/gif"},
        std::pair {".bmp",     "image/bmp"},
        std::pair {".ico",     "image/vnd.microsoft.icon"},
        std::pair {".tiff",    "image/tiff"},
        std::pair {".tif",     "image/tiff"},
        std::pair {".svg",     "image/svg+xml"},
        std::pair {".svgz",    "image/svg+xml"}
    }; 
    auto extension = std::filesystem::path {path}.extension().string();
    auto index = fileMap.find(extension);
    if (index == fileMap.end()){return "application/text";}
    return index->second; 
}

// Append an HTTP relative-path to a local filesystem path.
// The returned path is normalized for the platform.
std::string createPath(boost::beast::string_view base,
                       boost::beast::string_view path)
{
    std::string result(base);
#ifdef BOOST_MSVC
    constexpr char pathSeparator = '\\';
    if (result.back() == pathSeparator)
    {
        result.resize(result.size() - 1);
    }
    result.append(path.data(), path.size());
    for (auto &c : result)
    {
        if (c == '/'){c = pathSeparator;}
    }
#else
    constexpr char pathSeparator = '/';
    if (result.back() == pathSeparator)
    {
        result.resize(result.size() - 1);
    }
    result.append(path.data(), path.size());
#endif
    return result;
}

// Return a response for the given request.
//
// The concrete type of the response message (which depends on the
// request), is type-erased in message_generator.
template <class Body, class Allocator>
boost::beast::http::message_generator
handleRequest(
    boost::beast::string_view documentRoot,
    boost::beast::http::request
    <
       Body, boost::beast::http::basic_fields<Allocator>
    > &&request,
    US8::Services::WebServer::CallbackHandler &callbackHandler)
{
    // Returns a bad request response
    const auto badRequest = [&request](boost::beast::string_view why)
    {
        spdlog::info("Bad request");
        boost::beast::http::response<boost::beast::http::string_body> result
        {
            boost::beast::http::status::bad_request,
            request.version()
        };
#ifdef ENABLE_CORS
        result.set(boost::beast::http::field::access_control_allow_origin, "*");
#endif
        result.set(boost::beast::http::field::server,
                   BOOST_BEAST_VERSION_STRING);
        result.set(boost::beast::http::field::content_type,
                   "application/json");
        result.keep_alive(request.keep_alive());
        result.body() = "{\"status\":\"error\", \"reason\":\""
                      + std::string(why)
                      + "\"}";
        result.prepare_payload();
        return result;
    };

    // Returns a invalid permission (403) response
    const auto forbidden = [&request](boost::beast::string_view why)
    {
        spdlog::info("Forbidden");
        boost::beast::http::response<boost::beast::http::string_body> result
        {
            boost::beast::http::status::forbidden,
            request.version()
        };
#ifdef ENABLE_CORS
        result.set(boost::beast::http::field::access_control_allow_origin, "*");
#endif
        result.set(boost::beast::http::field::server,
                   BOOST_BEAST_VERSION_STRING);
        result.set(boost::beast::http::field::content_type,
                   "application/json");
        result.keep_alive(request.keep_alive());
        result.body() = "{\"status\":\"error\",\"reason\":\""
                      + std::string(why)
                      + "\"}";
        result.prepare_payload();
        return result;
    }; 

    // Returns an unimplemented (501) response
    const auto unimplemented = [&request](boost::beast::string_view why)
    {
        spdlog::info("Unimplemented");
        boost::beast::http::response<boost::beast::http::string_body> result
        {
            boost::beast::http::status::not_implemented,
            request.version()
        };
#ifdef ENABLE_CORS
        result.set(boost::beast::http::field::access_control_allow_origin, "*");
#endif
        result.set(boost::beast::http::field::server,
                   BOOST_BEAST_VERSION_STRING);
        result.set(boost::beast::http::field::content_type,
                   "text/html");
        result.keep_alive(request.keep_alive());
        result.body() = std::string(why);
        result.prepare_payload();
        return result;
    };

    // Returns a not found response
    const auto notFound = [&request](boost::beast::string_view target)
    {
        spdlog::debug("No packets found corresponding to request: "
                    + std::string{target});
        boost::beast::http::response<boost::beast::http::string_body> result
        {
            boost::beast::http::status::not_found,
            request.version()
        };
#ifdef ENABLE_CORS
        result.set(boost::beast::http::field::access_control_allow_origin, "*");
#endif
        result.set(boost::beast::http::field::server,
                   BOOST_BEAST_VERSION_STRING);
        result.set(boost::beast::http::field::content_type,
                   "text/html");
        result.keep_alive(request.keep_alive());
        result.body() = "Error 404: No data selected\n\nRequest:\n"
                      + std::string{target};
        result.prepare_payload();
        return result;
    };

    // Returns a no content response
    const auto noContent = [&request](boost::beast::string_view target)
    {
        spdlog::debug("No packets found corresponding to request: "
                    + std::string{target});
        boost::beast::http::response<boost::beast::http::string_body> result
        {
            boost::beast::http::status::no_content,
            request.version()
        };
#ifdef ENABLE_CORS
        result.set(boost::beast::http::field::access_control_allow_origin, "*");
#endif
        result.set(boost::beast::http::field::server,
                   BOOST_BEAST_VERSION_STRING);
        result.set(boost::beast::http::field::content_type,
                   "text/html");
        result.keep_alive(request.keep_alive());
        //result.body() = "The resource '" + std::string{target}
        //              + "' was not found.";
        result.prepare_payload();
        return result;
    };  

    // Returns an indication that user is not authorized
    const auto unauthorized = [&request](boost::beast::string_view target)
    {
        spdlog::info("Unauthorized");
        boost::beast::http::response<boost::beast::http::string_body> result
        {
            boost::beast::http::status::unauthorized,
            request.version()
        };
#ifdef ENABLE_CORS
        result.set(boost::beast::http::field::access_control_allow_origin, "*");
#endif
        result.set(boost::beast::http::field::server,
                   BOOST_BEAST_VERSION_STRING);
        result.set(boost::beast::http::field::content_type,
                   "application/json");
        result.keep_alive(request.keep_alive());
        result.body() = "{\"status\":\"error\",\"reason\":\""
                      + std::string(target)
                      + "\"}";
        result.prepare_payload();
        return result;
    };

    // Options request for CORS
    const auto optionsHandler = [&request]()
    {
        spdlog::debug("CORS");
        boost::beast::http::response<boost::beast::http::string_body> result
        {
            boost::beast::http::status::no_content,
            request.version()
        };
#ifdef ENABLE_CORS
        result.set(boost::beast::http::field::access_control_allow_origin, "*");
#endif
        result.set("Access-Control-Allow-Credentials",
                   "true");
        result.set(boost::beast::http::field::access_control_allow_methods,
                   "GET,HEAD,OPTIONS,POST,PUT");
        result.set(boost::beast::http::field::access_control_allow_headers,
                   "Access-Control-Allow-Origin, Access-Control-Allow-Headers, Access-Control-Allow-Methods, Connection, Origin, Accept, X-Requested-With, Content-Type, Access-Control-Request-Method, Access-Control-Request-Headers, Authorization");
        result.set(boost::beast::http::field::access_control_max_age,
                   "3600");
        result.set(boost::beast::http::field::connection,
                   "close");
        result.set(boost::beast::http::field::server,
                   BOOST_BEAST_VERSION_STRING);
        result.set(boost::beast::http::field::content_type,
                   "text/html");
        result.keep_alive(request.keep_alive());
        result.body() = "";
        result.prepare_payload();
        return result;
    };

    // Returns a server error response
    const auto serverError = [&request](boost::beast::string_view what)
    {
        spdlog::info("WebServer error");
        boost::beast::http::response<boost::beast::http::string_body> result
        {
            boost::beast::http::status::internal_server_error,
            request.version()
        };
#ifdef ENABLE_CORS
        result.set(boost::beast::http::field::access_control_allow_origin, "*");
#endif
        result.set(boost::beast::http::field::server,
                   BOOST_BEAST_VERSION_STRING);
        result.set(boost::beast::http::field::content_type,
                   "application/json");
        result.keep_alive(request.keep_alive());
        result.body() = "{\"status\":\"error\",\"reason\":\""
                      + std::string(what)
                      + "\"}";
        result.prepare_payload();
        return result;
    };

    // This comes up during CORS weirdness.  Basically, we need to tell
    // the browser all the headers the backend will accept.
    if (request.method() == boost::beast::http::verb::options)
    { 
        return optionsHandler();
    }

    // Make sure we can handle the method
    if (request.method() != boost::beast::http::verb::get)
    {
        return badRequest("Unknown HTTP-method");
    }

    // Process the put/post/get request
    if (request.method() == boost::beast::http::verb::get)
    {
        namespace UWebServer = US8::Services::WebServer;
/*
        try
        {
            auto [payload, mimeType]
                = callback(request.base(),
                           request.body(),
                           request.method());
            boost::beast::http::response<boost::beast::http::string_body> result
            {
                boost::beast::http::status::ok,
                request.version()
            };
#ifdef ENABLE_CORS
            result.set(boost::beast::http::field::access_control_allow_origin, "*");
#endif
            result.set(boost::beast::http::field::server,
                       BOOST_BEAST_VERSION_STRING);
            result.set(boost::beast::http::field::content_type,
                       mimeType);
            result.keep_alive(request.keep_alive());
            result.body() = payload;
            result.prepare_payload();
            return result;
        }
        catch (const UWebServer::Exception::InvalidPermission &e)
        {
            return forbidden(e.what());
        }
        catch (const UWebServer::Exception::Unimplemented &e)
        {
            return unimplemented(e.what());
        }
        catch (const UWebServer::Exception::NoContent &e)
        {
            return noContent(e.what());
        }
        catch (const UWebServer::Exception::NotFound &e)
        {
            return notFound(e.what());
        }
        catch (const std::invalid_argument &e)
        {
            return badRequest(e.what());
        }
        catch (const std::exception &e)
        {
            return serverError(e.what());
        }
*/
    }
    return serverError("Unhandled method");
}

///--------------------------------------------------------------------------///
///                               Web Sockets                                ///
///--------------------------------------------------------------------------///
// Echoes back all received WebSocket messages.
// This uses the Curiously Recurring Template Pattern so that
// the same code works with both SSL streams and regular sockets.
template<class Derived>
class WebSocketSession
{
public:
    // Start the asynchronous operation
    template<class Body, class Allocator>
    void run(boost::beast::http::request
             <
              Body,
              boost::beast::http::basic_fields<Allocator>
             > request)
    {
        // Accept the WebSocket upgrade request
        doAccept(std::move(request));
    }
private:
    // Access the derived class, this is part of
    // the Curiously Recurring Template Pattern idiom.
    Derived& derived()
    {
        return static_cast<Derived &> (*this);
    }

    // Start the asynchronous operation
    template<class Body, class Allocator>
    void doAccept(boost::beast::http::request
                  <
                   Body,
                   boost::beast:: http::basic_fields<Allocator>
                  > request)
    {
        // Set suggested timeout settings for the websocket
        derived().ws().set_option(
            boost::beast::websocket::stream_base::timeout::suggested(
                boost::beast::role_type::server));

        // Set a decorator to change the Server of the handshake
        derived().ws().set_option(
            boost::beast::websocket::stream_base::decorator(
            [](boost::beast::websocket::response_type &result)
            {
                result.set(boost::beast::http::field::server,
                    std::string(BOOST_BEAST_VERSION_STRING)
                  + " KUS8");// + mServiceName);
            }));

        // Enable deflation on response messages.  Requests are small.
        boost::beast::websocket::permessage_deflate deflateMessageOption;
        //deflateMessageOption.msg_size_threshold = 0; // Bytes (0 is default)
        deflateMessageOption.client_enable = false; // Clients
        deflateMessageOption.server_enable = true;  // Servers (me!)
        derived().ws().set_option(deflateMessageOption);

        // Accept the websocket handshake
        derived().ws().async_accept(
            request,
            boost::beast::bind_front_handler(
                &WebSocketSession::onAccept,
                derived().shared_from_this()));
    }

    void onAccept(boost::beast::error_code errorCode)
    {
        if (errorCode)
        {
            spdlog::critical("WebSocketSession::onAccept failed with "
                           + std::string {errorCode.what()});
            return;
        }

        // Read a message
        doRead();
    }

    void onRead(
        boost::beast::error_code errorCode,
        const size_t bytesTransferred)
    {
        boost::ignore_unused(bytesTransferred);

        // This indicates that the websocket_session was closed
        if (errorCode == boost::beast::websocket::error::closed)
        {
            spdlog::info("WebSocketSession::onRead socket closed");
            return;
        }

        if (errorCode)
        {
            spdlog::warn("WebSocketSession::onRead failed with "
                       + std::string {errorCode.what()});
            return;
        }

        // Get the request message
        derived().ws().text(derived().ws().got_text());
        auto requestMessage
             = boost::beast::buffers_to_string(mReadBuffer.data());
        mReadBuffer.consume(mReadBuffer.size());
        
        // Attempt to do something with the thread
        try
        {
            auto responseMessage
                = derived().getCallbackHandler()->process(requestMessage);
            if (responseMessage)
            {
                reply(US8::Services::WebServer::Messages::toJSON(responseMessage));
            }
            else
            {
                throw std::runtime_error("Callback returned a NULL message");
            }
        }
        catch (const std::exception &e)
        {
            spdlog::warn("WebSocketSession::onRead reply failed with "
                       + std::string{e.what()});
            try
            {
                US8::Services::WebServer::Messages::Error errorMessage;
                errorMessage.setStatusCode(500);
                errorMessage.setMessage("server error - unhandled exception");
                reply(US8::Services::WebServer::Messages::toJSON(*errorMessage.clone()));
            }
            catch (const std::exception &e)
            {
                spdlog::error("WebSocketSession::onRead error failed with "
                            + std::string {e.what()});
            }
        }

        // Go back to reading 
        derived().ws().async_read(
            mReadBuffer,
            boost::beast::bind_front_handler(
                &WebSocketSession::onRead,
                derived().shared_from_this()));
    }

    void reply(const std::string &responseString)
    {
        reply(std::make_shared<std::string> (responseString));
    }

    void reply(const std::shared_ptr<const std::string> &stringStream)
    {
        // Post our work to the strand, this ensures that the members of `this'
        // will not be accessed concurrently.
        boost::asio::post
        (
            derived().ws().get_executor(),
            boost::beast::bind_front_handler
            (
                &WebSocketSession::queueSend,
                derived().shared_from_this(),
                stringStream
            )
        );

        // Fall through to on read  
    }

    void queueSend(const std::shared_ptr<const std::string> &response)
    {
        // Allocate and store the message
        mResponseQueue.push(std::move(response));

        // Are we already writing?
        if (mResponseQueue.size() > 1)
        {
            spdlog::debug("WebSocketSession: queueSend is already writing...");
            return;
        }

        // We are not currently writing so send this message
        if (!mResponseQueue.empty())
        {
            derived().ws().async_write(
                boost::asio::buffer(std::move(*mResponseQueue.front())),
                boost::beast::bind_front_handler(
                   &WebSocketSession::onWrite,
                   derived().shared_from_this()));
        }

        // Fall through to reply 
    }

    void onWrite(boost::beast::error_code errorCode,
                 const size_t bytesTransferred)
    {
        boost::ignore_unused(bytesTransferred);

        if (errorCode)
        {
            spdlog::critical("WebSocketSession::onWrite failed with "
                           + std::string {errorCode.what()});
            return;
        }

        // Remove the latest sent message from the response queue
        mResponseQueue.pop();
 
        // Send the next message if possible
        if (!mResponseQueue.empty()) 
        {
            derived().ws().async_write(
                boost::asio::buffer(std::move(*mResponseQueue.front())),
                boost::beast::bind_front_handler(
                   &WebSocketSession::onWrite,
                   derived().shared_from_this()));
        }

        // Fall through to queueSend 
    }

    void doRead()
    {
        // Read a message into our buffer
        derived().ws().async_read(
            mReadBuffer,
            boost::beast::bind_front_handler(
                &WebSocketSession::onRead,
                derived().shared_from_this()));
    }

    boost::beast::flat_buffer mReadBuffer;
    boost::beast::flat_buffer mWriteBuffer;
    std::queue<std::shared_ptr<const std::string>> mResponseQueue;
    bool mWriting{false};
};

// Handles a plain WebSocket connection
class PlainWebSocketSession :
    public ::WebSocketSession<::PlainWebSocketSession>,
    public std::enable_shared_from_this<::PlainWebSocketSession>
{
public:
    // Create the session
    PlainWebSocketSession(
        const std::string sessionIdentifier, 
        boost::beast::tcp_stream &&stream,
        std::shared_ptr<US8::Services::WebServer::CallbackHandler>
            &callbackHandler) :
        mWebSocket(std::move(stream)),
        mCallbackHandler(callbackHandler),
        mSessionIdentifier(sessionIdentifier)
    {
        //mWebSocket.binary(true);
    }

    ~PlainWebSocketSession()
    {
        spdlog::info("PlainWebSocketSession: Terminating session for "
                   + mSessionIdentifier);
    }       

    // Called by the base class to get a handle to the websocket
    boost::beast::websocket::stream<boost::beast::tcp_stream> &ws()
    {
        return mWebSocket;
    }

    // Called by the base class to get a handle to the callback handler
    std::shared_ptr<US8::Services::WebServer::CallbackHandler> getCallbackHandler()
    {
        return mCallbackHandler;
    }
private:
    boost::beast::websocket::stream<boost::beast::tcp_stream> mWebSocket;
    std::shared_ptr<US8::Services::WebServer::CallbackHandler>
        mCallbackHandler{nullptr};
    std::string mSessionIdentifier;
    std::string mServiceName{"us8"};
};

// Handles an SSL WebSocket connection
class SSLWebSocketSession :
    public ::WebSocketSession<::SSLWebSocketSession>,
    public std::enable_shared_from_this<::SSLWebSocketSession>
{
public:
    // Create the ssl_websocket_session
    SSLWebSocketSession(
        const std::string &sessionIdentifier,
        boost::asio::ssl::stream<boost::beast::tcp_stream> &&stream,
        std::shared_ptr<US8::Services::WebServer::CallbackHandler> &callbackHandler) :
        mWebSocket(std::move(stream)),
        mCallbackHandler(callbackHandler),
        mSessionIdentifier(sessionIdentifier)
    {
        //mWebSocket.binary(true);
    }

    ~SSLWebSocketSession()
    {
        spdlog::info("SSLWebSocketSession: Terminating session for: " 
                   + mSessionIdentifier);
    }

    // Called by the base class
    boost::beast::websocket::stream
    <
     boost::asio::ssl::stream<boost::beast::tcp_stream>
    > &ws()
    {
        return mWebSocket;
    }

    std::shared_ptr<US8::Services::WebServer::CallbackHandler>
    getCallbackHandler()
    {
        return mCallbackHandler;
    }
private:
    boost::beast::websocket::stream
    <
     boost::asio::ssl::stream<boost::beast::tcp_stream>
    > mWebSocket;
    std::shared_ptr<US8::Services::WebServer::CallbackHandler>
        mCallbackHandler{nullptr};
    std::string mSessionIdentifier;
    std::string mServiceName{"us8"};
};

//------------------------------------------------------------------------------

// Handles an HTTP server connection.
// This uses the Curiously Recurring Template Pattern so that
// the same code works with both SSL streams and regular sockets.
template<class Derived>
class Session
{
public:
    // Take ownership of the buffer
    Session(
        boost::beast::flat_buffer buffer,
        const std::shared_ptr<const std::string> &documentRoot,
        std::shared_ptr<US8::Services::WebServer::CallbackHandler>
            &callbackHandler) :
        mDocumentRoot(documentRoot),
        mBuffer(std::move(buffer)),
        mCallbackHandler(callbackHandler)
    {
    }

    void doRead()
    {
        // Construct a new parser for each message
        mRequestParser.emplace();
        mRequestParser->body_limit(2048);

        // Set the timeout.
        boost::beast::get_lowest_layer(
            derived().stream()).expires_after(std::chrono::seconds(30));

        // Read a request
        boost::beast::http::async_read(
            derived().stream(),
            mBuffer,
            //mRequest,
            *mRequestParser,
            boost::beast::bind_front_handler(
                &Session::onRead,
                derived().shared_from_this()));
    }
private:
    void onRead(
        boost::beast::error_code errorCode,
        const size_t bytesTransferred)
    {
        boost::ignore_unused(bytesTransferred);

        // This means they closed the connection
        if (errorCode == boost::beast::http::error::end_of_stream)
        {
            return derived().closeConnection();
        }

        if (errorCode)
        {
            if (errorCode != boost::asio::ssl::error::stream_truncated)
            {
                spdlog::critical("Session::onRead read failed with "
                               + std::string {errorCode.what()});
            }
            return;
        }

        // Send the response
        sendResponse(::handleRequest(*mDocumentRoot,
                                     //std::move(mRequest),
                                     mRequestParser->release(),
                                     *mCallbackHandler));
    }

    void sendResponse(boost::beast::http::message_generator &&message)
    {
        bool keepAlive = message.keep_alive();

        // Write the response
        boost::beast::async_write(
            derived().stream(),
            std::move(message),
            boost::beast::bind_front_handler(
                &Session::onWrite,
                derived().shared_from_this(),
                keepAlive));
    }

    void onWrite(const bool keepAlive,
                 boost::beast::error_code errorCode,
                 const size_t bytesTransferred)
    {
        boost::ignore_unused(bytesTransferred);

        if (errorCode)
        {
            if (errorCode != boost::asio::ssl::error::stream_truncated)
            {
                spdlog::critical("Session::onWrite write failed with "
                               + std::string {errorCode.what()});
            }
            return;
        }

        if (!keepAlive)
        {
            // This means we should close the connection, usually because
            // the response indicated the "Connection: close" semantic.
            return derived().closeConnection();
        }

        // Read another request
        doRead();
    }

protected:
    boost::beast::flat_buffer mBuffer;

private:
    // Access the derived class, this is part of
    // the Curiously Recurring Template Pattern idiom.
    Derived& derived()
    {
        return static_cast<Derived &> (*this);
    }

    std::shared_ptr<const std::string> mDocumentRoot;
    //boost::beast::http::request<boost::beast::http::string_body> mRequest;
    // The parser is stored in an optional container so we can
    // construct it from scratch it at the beginning of each new message.
    boost::optional
    <    
     boost::beast::http::request_parser<boost::beast::http::string_body>
    > mRequestParser;
    std::shared_ptr<US8::Services::WebServer::CallbackHandler> mCallbackHandler{nullptr};
};

// Handles a plain HTTP connection
class PlainSession : public Session<::PlainSession>,
                     public std::enable_shared_from_this<::PlainSession>
{
public:
    // Create the session
    PlainSession(
        boost::asio::ip::tcp::socket&& socket,
        boost::beast::flat_buffer buffer,
        const std::shared_ptr<const std::string> &documentRoot,
        std::shared_ptr<US8::Services::WebServer::CallbackHandler>
             &callbackHandler) :
        ::Session<::PlainSession>(
            std::move(buffer),
            documentRoot,
            callbackHandler),
        mStream(std::move(socket))
    {
    }

    // Called by the base class
    boost::beast::tcp_stream& stream()
    {
        return mStream;
    }

    // Start the asynchronous operation
    void run()
    {
        // We need to be executing within a strand to perform async operations
        // on the I/O objects in this session. Although not strictly necessary
        // for single-threaded contexts, this example code is written to be
        // thread-safe by default.
        boost::asio::dispatch(mStream.get_executor(),
                              boost::beast::bind_front_handler(
                                 &Session::doRead,
                                 shared_from_this()));
    }

    void closeConnection()
    {
        // Send a TCP shutdown
        boost::beast::error_code errorCode;
        mStream.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_send,
                                  errorCode);

        // At this point the connection is closed gracefully
    }
 
private:
    boost::beast::tcp_stream mStream;
};

// Handles an SSL HTTP connection
class SSLSession : public Session<::SSLSession>,
                   public std::enable_shared_from_this<::SSLSession>
{
public:
    // Create the session
    SSLSession(
        boost::asio::ip::tcp::socket &&socket,
        boost::asio::ssl::context &sslContext,
        boost::beast::flat_buffer buffer,
        const std::shared_ptr<const std::string> &documentRoot,
        std::shared_ptr<US8::Services::WebServer::CallbackHandler>
            &callbackHandler) :
        ::Session<::SSLSession>(std::move(buffer),
                                documentRoot,
                                callbackHandler),
        mStream(std::move(socket), sslContext)
    {
    }

    // Called by the base class
    boost::beast::ssl_stream<boost::beast::tcp_stream> &stream()
    {
        return mStream;
    }

    // Start the asynchronous operation
    void run()
    {
        auto self = shared_from_this();
        // We need to be executing within a strand to perform async operations
        // on the I/O objects in this session.
        boost::asio::dispatch(mStream.get_executor(), [self]() {
            // Set the timeout.
            boost::beast::get_lowest_layer(self->mStream).expires_after(
                std::chrono::seconds(30));

            // Perform the SSL handshake
            // Note, this is the buffered version of the handshake.
            self->mStream.async_handshake(
                boost::asio::ssl::stream_base::server,
                self->mBuffer.data(),
                boost::beast::bind_front_handler(
                    &::SSLSession::onHandshake,
                    self));
        });
    }

    void onHandshake(boost::beast::error_code errorCode,
                     const size_t bytesUsed)
    {
        if (errorCode)
        {
            if (errorCode != boost::asio::ssl::error::stream_truncated)
            {
                spdlog::critical(
                    "SSLSession::onHandhsake handshake failed with: "
                   + std::string {errorCode.what()});
            }
            return;
        }

        // Consume the portion of the buffer used by the handshake
        mBuffer.consume(bytesUsed);

        doRead();
    }

    void closeConnection()
    {
        // Set the timeout.
        boost::beast::get_lowest_layer(mStream).
            expires_after(std::chrono::seconds(30));

        // Perform the SSL shutdown
        mStream.async_shutdown(
            boost::beast::bind_front_handler(
                &::SSLSession::onShutdown,
                shared_from_this()));
    }

    void onShutdown(boost::beast::error_code errorCode)
    {
        if (errorCode)
        {
            if (errorCode != boost::asio::ssl::error::stream_truncated)
            {
                spdlog::critical("SSLSession::onShutdown failed with: "
                               + std::string {errorCode.what()});
            }
            return;
        }

        // At this point the connection is closed gracefully
    }
private:
    boost::beast::ssl_stream<boost::beast::tcp_stream> mStream;
};

//------------------------------------------------------------------------------

// Detects SSL handshakes
class DetectSession : public std::enable_shared_from_this<::DetectSession>
{
public:
    DetectSession(
        boost::asio::ip::tcp::socket &&socket,
        boost::asio::ssl::context& sslContext,
        const std::shared_ptr<const std::string> &documentRoot,
        std::shared_ptr<US8::Services::WebServer::CallbackHandler>
            &callbackHandler) :
        mStream(std::move(socket)),
        mSSLContext(sslContext),
        mDocumentRoot(documentRoot),
        mCallbackHandler(callbackHandler)
    {
    }

    // Launch the detector
    void run()
    {
        // Set the timeout.
        boost::beast::get_lowest_layer(mStream)
           .expires_after(std::chrono::seconds(30));

        // Detect a TLS handshake
        boost::beast::async_detect_ssl(
            mStream,
            mBuffer,
            boost::beast::bind_front_handler(
                &::DetectSession::onDetect,
                shared_from_this()));
    }

    void onDetect(boost::beast::error_code errorCode, const bool result)
    {
        if (errorCode)
        {
            if (errorCode != boost::asio::ssl::error::stream_truncated)
            {
                spdlog::critical(
                   "DetectSession::onDetect: Failed to detect; failed with: "
                  + std::string {errorCode.what()});
            }
            return;
        }

        if (result)
        {
            // Launch SSL session
            std::make_shared<::SSLSession>(
                mStream.release_socket(),
                mSSLContext,
                std::move(mBuffer),
                mDocumentRoot,
                mCallbackHandler)->run();
            return;
        }

        // Launch plain session
        std::make_shared<::PlainSession>(
            mStream.release_socket(),
            std::move(mBuffer),
            mDocumentRoot,
            mCallbackHandler)->run();
    }

private:
    boost::beast::tcp_stream mStream;
    boost::asio::ssl::context& mSSLContext;
    std::shared_ptr<const std::string> mDocumentRoot;
    boost::beast::flat_buffer mBuffer;
    std::shared_ptr<US8::Services::WebServer::CallbackHandler>
        mCallbackHandler{nullptr};
};

}
#endif
