#include <iostream>
#include <chrono>
#include <spdlog/spdlog.h>
#include <thread>
#include <zmq.hpp>
#include <cerrno>
#include <boost/program_options.hpp>
#include "clientOptions.hpp"
#include "streamSelector.hpp"
#include "us8/messageFormats/broadcasts/dataPacket.hpp"

#define PROXY_FRONTEND_ADDRESS "tcp://127.0.0.1:5555"

struct ProgramOptions
{
    std::string proxyFrontendAddress{PROXY_FRONTEND_ADDRESS};
    // Maximum time before a send operation returns with EAGAIN
    // -1 waits forever whereas 0 returns immediately.
    std::chrono::milliseconds sendTimeOut{1000}; // 1s is enough
    int sendHighWaterMark{1024};
    int verbosity{3};
};

::ProgramOptions parseCommandLineOptions(int argc, char *argv[]);

class Process
{
public:
    Process() = delete;
    explicit Process(const ProgramOptions &options)
    {
        try
        {
            if (options.sendHighWaterMark >= 0)
            {
                mPublisherSocket.set(zmq::sockopt::sndhwm,
                                     options.sendHighWaterMark);
            }
            auto timeOutMilliSeconds
                = static_cast<int> (options.sendTimeOut.count());
            if (timeOutMilliSeconds >= 0)
            {
                mPublisherSocket.set(zmq::sockopt::sndtimeo,
                                     timeOutMilliSeconds);
            }
            mPublisherSocket.connect(options.proxyFrontendAddress);
        }
        catch (const std::exception &e)
        {
            auto errorMessage = "Failed to initialize publisher socket because "
                              + std::string {e.what()};
            throw std::runtime_error(errorMessage);
        }
    }
    void sendPacket(const US8::MessageFormats::Broadcasts::DataPacket &packet)
    {
        try
        {
            auto messageContainer = packet.serialize();
            zmq::message_t message{messageContainer.data(), messageContainer.size()};
            mPublisherSocket.send(std::move(message),
                                  zmq::send_flags::dontwait);
        }
        catch (const std::exception &e)
        {
            spdlog::warn("Failed to send message because "
                        + std::string {e.what()});
        }
    }
    void callback(US8::MessageFormats::Broadcasts::DataPacket &&packet)
    {
        // Check the packet 
    }
    std::thread mPublisherThread;
    zmq::context_t mPublisherContext{1};
    zmq::socket_t mPublisherSocket{mPublisherContext, zmq::socket_type::pub}; 
};

int main(int argc, char *argv[])
{
    ::ProgramOptions programOptions;
/*
    try 
    {   
        programOptions = ::parseCommandLineOptions(argc, argv);
        if (programOptions.helpOnly){return EXIT_SUCCESS;}
    }   
    catch (const std::exception &e) 
    {   
        spdlog::error(e.what());
        return EXIT_FAILURE;
    }   
*/


    // Receive from our publishers
    spdlog::info("Creating ZMQ publisher");
    auto zmqContext = zmq_ctx_new();
    auto publisherSocket = zmq_socket(zmqContext, ZMQ_PUB);
    spdlog::info("Connecting to proxy frontend at " 
               + programOptions.proxyFrontendAddress);
    auto response = zmq_connect(publisherSocket,
                                programOptions.proxyFrontendAddress.c_str());
    if (response != 0)
    {
        spdlog::error("Failed to connect to proxy");
        return EXIT_SUCCESS;
    }


    for (int i = 0; i < 20; ++i)
    {
        std::string zmqMessage{"abc123"};
        auto bytesSent = zmq_send(publisherSocket, zmqMessage.c_str(), zmqMessage.size(), 0);
        spdlog::info("Sent " + std::to_string(bytesSent));
        std::this_thread::sleep_for(std::chrono::seconds {1});
    }
    response = zmq_close(publisherSocket);
    if (response != 0)
    {
        spdlog::warn("Failed to close socket");
    }
    zmq_ctx_destroy(zmqContext);
    return EXIT_SUCCESS;
}
