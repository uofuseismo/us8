#include <iostream>
#include <atomic>
#include <string>
#include <thread>
#include <filesystem>
#include <cerrno>
#include <spdlog/spdlog.h>
#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <readerwriterqueue.h>
#include <zmq.hpp>
#include "us8/broadcasts/dataPacket/client.hpp"
#include "us8/messageFormats/broadcasts/dataPacket.hpp"
#include "testFutureDataPacket.hpp"
#include "testExpiredDataPacket.hpp"

#define RAW_DATA_PACKET_BROADCAST_BACKEND_ADDRESS "tcp://127.0.0.1:5551"
#define SANITIZED_DATA_PACKET_BROADCAST_FRONTEND_ADDRESS "tcp://127.0.0.1:5552"

#define MAX_QUEUE_SIZE 256

std::pair<std::string, bool> parseCommandLineOptions(int argc, char *argv[]);

struct ProgramOptions
{
    std::string proxyBackendAddress{RAW_DATA_PACKET_BROADCAST_BACKEND_ADDRESS};
    std::string proxyFrontendAddress{
        SANITIZED_DATA_PACKET_BROADCAST_FRONTEND_ADDRESS};
    std::chrono::milliseconds receiveTimeOut{10};
    std::chrono::milliseconds sendTimeOut{1000}; // 1s is enough
    std::chrono::microseconds maxFutureTime{0};
    std::chrono::seconds maxLatency{120};
    std::chrono::seconds logBadDataInterval{3600};
    int receiveHighWaterMark{4096};
    int sendHighWaterMark{4096};
};

class Process
{
public:
    explicit Process(const ::ProgramOptions &programOptions) :
        futureDataPacketTester{programOptions.maxFutureTime,
                               programOptions.logBadDataInterval},
        expiredDataPacketTester{programOptions.maxLatency,
                                programOptions.logBadDataInterval}
    {
        // Initialize ZMQ subscriber
        try
        {
            if (programOptions.receiveHighWaterMark >= 0)
            {
                mSubscriberSocket.set(zmq::sockopt::rcvhwm,
                                     programOptions.receiveHighWaterMark);
            }
            auto timeOutMilliSeconds
                = static_cast<int> (programOptions.receiveTimeOut.count());
            if (timeOutMilliSeconds >= 0)
            {
                mSubscriberSocket.set(zmq::sockopt::rcvtimeo,
                                       timeOutMilliSeconds);
            }
            spdlog::info("Connecting to proxy backend address "
                       + programOptions.proxyBackendAddress);
            mSubscriberSocket.connect(programOptions.proxyBackendAddress);
        }
        catch (const std::exception &e) 
        {
            auto errorMessage = "Failed to initialize subscriber socket because "
                              + std::string {e.what()};
            throw std::runtime_error(errorMessage);
        }

        // Initialize ZMQ publisher
        try
        {
            if (programOptions.sendHighWaterMark >= 0)
            {
                mPublisherSocket.set(zmq::sockopt::sndhwm,
                                     programOptions.sendHighWaterMark);
            }
            auto timeOutMilliSeconds
                = static_cast<int> (programOptions.sendTimeOut.count());
            if (timeOutMilliSeconds >= 0)
            {   
                mPublisherSocket.set(zmq::sockopt::sndtimeo,
                                     timeOutMilliSeconds);
            }
            spdlog::info("Connecting to proxy frontend address "
                       + programOptions.proxyFrontendAddress);
            mPublisherSocket.connect(programOptions.proxyFrontendAddress);
        }
        catch (const std::exception &e) 
        {
            auto errorMessage = "Failed to initialize publisher socket because "
                              + std::string {e.what()};
            throw std::runtime_error(errorMessage);
        }
    }
    /// Stops the threads
    void stop()
    {   
        mKeepRunning = false; 
        if (mSubscriberThread.joinable()){mSubscriberThread.join();}
        if (mTesterThread.joinable()){mTesterThread.join();}
        if (mPublisherThread.joinable()){mPublisherThread.join();}
    }   
    /// Reads the packets from the wire to test

    /// Publishes the good packets
    void publishGoodPackets()
    {
        spdlog::info("Thread entering publisher");
        std::chrono::milliseconds sleepTime{5};
        auto nowMuSeconds
           = std::chrono::time_point_cast<std::chrono::microseconds>
             (std::chrono::high_resolution_clock::now()).time_since_epoch();
        auto lastLogTime
            = std::chrono::duration_cast<std::chrono::seconds> (nowMuSeconds);
        int64_t nSentPackets{0};
        int64_t nNotSentPackets{0};
        while (mKeepRunning)
        {
            if (mMessageToPublishQueue.size_approx() > MAX_QUEUE_SIZE)
            {
                int nDeleted{0};
                while (mMessageToPublishQueue.size_approx() > MAX_QUEUE_SIZE)
                {
                    if (!mMessageToPublishQueue.pop()){break;}
                    nDeleted = nDeleted + 1;
                }
                spdlog::warn("Overfull publisher queue - deleted "
                           + std::to_string(nDeleted) + " packets");
                nNotSentPackets = nNotSentPackets + nDeleted;
            }
            auto packet = mMessageToPublishQueue.peek();
            if (packet)
            {
                std::string messageContainer;
                try
                {
                    messageContainer = packet->serialize();
                }
                catch (const std::exception &e) 
                {
                    spdlog::warn("Failed to serialize output message because "
                               + std::string {e.what()});
                }
                if (!mMessageToPublishQueue.pop())
                {
                    spdlog::warn("Publisher queue appears to be empty");
                }
                try
                {
                    zmq::message_t message{messageContainer.data(),
                                           messageContainer.size()};
                    mPublisherSocket.send(std::move(message),
                                          zmq::send_flags::dontwait);
                    nSentPackets = nSentPackets + 1;
                }
                catch (const std::exception &e)
                {
                    spdlog::warn("Failed to send message because "
                               + std::string {e.what()});
                    nNotSentPackets = nNotSentPackets + 1;
                }
            }
            else
            {   
                std::this_thread::sleep_for(sleepTime);
            }
            nowMuSeconds
                = std::chrono::time_point_cast<std::chrono::microseconds>
                 (std::chrono::high_resolution_clock::now()).time_since_epoch();
            auto nowSeconds
                = std::chrono::duration_cast<std::chrono::seconds>
                  (nowMuSeconds);
            if (nowSeconds >= lastLogTime + mLogPublishingPerformanceInterval)
            {
                spdlog::info("Sent " 
                    + std::to_string(nSentPackets)
                    + " packets in last "
                    + std::to_string(mLogPublishingPerformanceInterval.count())
                    + "seconds. (Failed to send " 
                    + std::to_string(nNotSentPackets) + " packets.)");
                nSentPackets = 0;
                nNotSentPackets = 0;
                lastLogTime = nowSeconds;
            }
        }
    } 
    Process(const Process &) = delete;
    Process& operator=(const Process &) = delete;
///private:
    std::thread mSubscriberThread;
    std::thread mTesterThread;
    std::thread mPublisherThread;
    zmq::context_t mSubscriberContext{1};
    zmq::context_t mPublisherContext{1};
    zmq::socket_t mSubscriberSocket{mPublisherContext, zmq::socket_type::sub};
    zmq::socket_t mPublisherSocket{mPublisherContext, zmq::socket_type::pub}; 
    US8::Broadcasts::DataPacket::Sanitizer::TestFutureDataPacket
        futureDataPacketTester;
    US8::Broadcasts::DataPacket::Sanitizer::TestExpiredDataPacket
        expiredDataPacketTester;
    moodycamel::ReaderWriterQueue<US8::MessageFormats::Broadcasts::DataPacket>
        mNewMessageQueue{MAX_QUEUE_SIZE};
    moodycamel::ReaderWriterQueue<US8::MessageFormats::Broadcasts::DataPacket>
        mMessageToPublishQueue{MAX_QUEUE_SIZE};
    std::chrono::seconds mLogPublishingPerformanceInterval{3600};
    std::atomic<bool> mKeepRunning{true};
};


///--------------------------------------------------------------------------///
///                                    Main Function                         ///
///--------------------------------------------------------------------------///

int main(int argc, char *argv[])
{
    // Get the ini file from the command line
    std::filesystem::path iniFile;
    try
    {
        auto [iniFileName, isHelp] = ::parseCommandLineOptions(argc, argv);
        if (isHelp){return EXIT_SUCCESS;}
        iniFile = iniFileName;
    }
    catch (const std::exception &e)
    {
        spdlog::error(e.what());
        return EXIT_FAILURE;
    }

/*
    // Read the program properties
    ::ProgramOptions programOptions;
    try
    {
        programOptions = parseIniFile(iniFile);
    }
    catch (const std::exception &e)
    {
        spdlog::error(e.what());
        return EXIT_FAILURE;
    }
    if (programOptions.verbosity <= 1){spdlog::set_level(spdlog::level::critical);}
    if (programOptions.verbosity == 2){spdlog::set_level(spdlog::level::warn);}
    if (programOptions.verbosity == 3){spdlog::set_level(spdlog::level::info);}
    if (programOptions.verbosity >= 4){spdlog::set_level(spdlog::level::debug);}
*/
}

///--------------------------------------------------------------------------///
///                            Utility Functions                             ///
///--------------------------------------------------------------------------///
/// Read the program options from the command line
std::pair<std::string, bool> parseCommandLineOptions(int argc, char *argv[])
{
    std::string iniFile;
    boost::program_options::options_description desc(R"""(
The dataPacketSanitizer reads data packets from a US8 broadcast and attempts
to remove - future, duplicate, and packets as well as packets that may
indicate a GPS clock slip.  Example usage is

    dataPacketSanitizer --ini=sanitizer.ini

Allowed options)""");
    desc.add_options()
        ("help", "Produces this help message")
        ("ini",  boost::program_options::value<std::string> (), 
                 "The initialization file for this executable");
    boost::program_options::variables_map vm; 
    boost::program_options::store(
        boost::program_options::parse_command_line(argc, argv, desc), vm); 
    boost::program_options::notify(vm);
    if (vm.count("help"))
    {    
        std::cout << desc << std::endl;
        return {iniFile, true};
    }   
    if (vm.count("ini"))
    {    
        iniFile = vm["ini"].as<std::string>();
        if (!std::filesystem::exists(iniFile))
        {
            throw std::runtime_error("Initialization file: " + iniFile
                                   + " does not exist");
        }
    }
    return {iniFile, false};
}

