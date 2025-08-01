#include <iostream>
#include <csignal>
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
//#include <zmq.hpp>
//#include <zmq_addon.hpp>
#include "us8/messageFormats/broadcasts/dataPacket.hpp"
#include "us8/broadcasts/dataPacket/publisher.hpp"
#include "us8/broadcasts/dataPacket/publisherOptions.hpp"
#include "us8/broadcasts/dataPacket/subscriber.hpp"
#include "us8/broadcasts/dataPacket/subscriberOptions.hpp"
#include "testFutureDataPacket.hpp"
#include "testExpiredDataPacket.hpp"
#include "testDuplicateDataPacket.hpp"

#define RAW_DATA_PACKET_BROADCAST_BACKEND_ADDRESS "tcp://127.0.0.1:5551"
#define SANITIZED_DATA_PACKET_BROADCAST_FRONTEND_ADDRESS "tcp://127.0.0.1:5552"

#define MAX_QUEUE_SIZE 256

namespace
{
std::atomic<bool> mInterrupted{false};
}
namespace USanitizer = US8::Broadcasts::DataPacket::Sanitizer;

struct ProgramOptions
{
    std::string inputBroadcastAddress{
        RAW_DATA_PACKET_BROADCAST_BACKEND_ADDRESS};
    std::string outputBroadcastAddress{
        SANITIZED_DATA_PACKET_BROADCAST_FRONTEND_ADDRESS};
    std::chrono::milliseconds receiveTimeOut{10};
    std::chrono::milliseconds sendTimeOut{1000}; // 1s is enough
    std::chrono::milliseconds maximumFutureTime{0};
    std::chrono::seconds maximumLatency{120};
    std::chrono::seconds circularBufferDuration{120};
    std::chrono::seconds logBadDataInterval{60};
    int receiveHighWaterMark{4096};
    int sendHighWaterMark{4096};
    int verbosity{3};
};

std::pair<std::string, bool> parseCommandLineOptions(int argc, char *argv[]);
::ProgramOptions parseIniFile(const std::filesystem::path &iniFile);

class Process
{
public:
    explicit Process(const ::ProgramOptions &programOptions) 
    {
        // Create the testers
        if (programOptions.maximumFutureTime.count() >= 0)
        {
            spdlog::info("Will test for future data");
            mFutureDataPacketTester
                = std::make_unique<USanitizer::TestFutureDataPacket>
                  (programOptions.maximumFutureTime,
                   programOptions.logBadDataInterval);
        }
        if (programOptions.maximumLatency.count() > 0)
        {
            spdlog::info("Will test for latent data older than "
                       + std::to_string(programOptions.maximumLatency.count())
                       + " seconds");
            mExpiredDataPacketTester
                = std::make_unique<USanitizer::TestExpiredDataPacket>
                  (programOptions.maximumLatency,
                   programOptions.logBadDataInterval);
        } 
        if (programOptions.circularBufferDuration.count() > 0)
        {
            spdlog::info("Will test for duplicate data");
            mDuplicateDataPacketTester
                = std::make_unique<USanitizer::TestDuplicateDataPacket>
                  (programOptions.circularBufferDuration,
                   programOptions.logBadDataInterval); 
        }

        // Initialize ZMQ subscriber
        try
        {
            US8::Broadcasts::DataPacket::SubscriberOptions subscriberOptions{
                programOptions.inputBroadcastAddress,
                mReceivePacketCallback};
            subscriberOptions.setHighWaterMark(
                programOptions.receiveHighWaterMark);
            subscriberOptions.setTimeOut(
                programOptions.receiveTimeOut);
    
            mPacketSubscriber
                = std::make_unique<US8::Broadcasts::DataPacket::Subscriber>
                  (subscriberOptions);
        }
        catch (const std::exception &e) 
        {
            auto errorMessage
                = "Failed to initialize subscriber socket because "
                + std::string {e.what()};
            throw std::runtime_error(errorMessage);
        }
        // Initialize ZMQ publisher
        try
        {
            US8::Broadcasts::DataPacket::PublisherOptions publisherOptions{
                programOptions.outputBroadcastAddress};
            publisherOptions.setHighWaterMark(
                programOptions.sendHighWaterMark);
            publisherOptions.setTimeOut(
                programOptions.sendTimeOut);
    
            mPacketPublisher
                = std::make_unique<US8::Broadcasts::DataPacket::Publisher>
                  (publisherOptions);
        }
        catch (const std::exception &e) 
        {
            auto errorMessage = "Failed to initialize publisher because "
                              + std::string {e.what()};
            throw std::runtime_error(errorMessage);
        }
    }
    /// Starts the threads
    void start()
    {
#ifndef NDEBUG
        assert(mPacketSubscriber != nullptr);
#endif
        stop();
        mKeepRunning = true;
        mPublisherThread = std::thread(&::Process::publishGoodPackets, this);
        mTesterThread = std::thread(&::Process::checkPackets, this);
        //mSubscriberThread = std::thread(&::Process::getInputPackets, this);
        mPacketSubscriber->start();
    }
    /// Stops the threads
    void stop()
    {   
        mKeepRunning = false; 
        if (mPacketSubscriber){mPacketSubscriber->stop();}
        //if (mSubscriberThread.joinable()){mSubscriberThread.join();}
        if (mTesterThread.joinable()){mTesterThread.join();}
        if (mPublisherThread.joinable()){mPublisherThread.join();}
    }   
    /// Callback for the packet subscriber
    void inputPacketsToQueueCallback(
        US8::MessageFormats::Broadcasts::DataPacket &&dataPacket)
    {
        mPacketsToCheckQueue.try_enqueue(std::move(dataPacket));
    }
    /// Checks the packets
    void checkPackets()
    {
        spdlog::debug("Thread entering checkPackets");
        constexpr std::chrono::milliseconds sleepTime{5};
        auto nowMuSeconds
           = std::chrono::time_point_cast<std::chrono::microseconds>
             (std::chrono::high_resolution_clock::now()).time_since_epoch();
        auto lastLogTime
            = std::chrono::duration_cast<std::chrono::seconds> (nowMuSeconds);
        int64_t nNotCheckedPackets{0};
        int64_t nCheckedPackets{0};
        while (mKeepRunning)
        {
            if (mPacketsToCheckQueue.size_approx() > MAX_QUEUE_SIZE)
            {
                int nDeleted{0};
                while (mPacketsToCheckQueue.size_approx() > MAX_QUEUE_SIZE)
                {
                    if (!mPacketsToCheckQueue.pop()){break;}
                    nDeleted = nDeleted + 1;
                }
                spdlog::warn("Overfull input queue - deleted "
                           + std::to_string(nDeleted) + " packets");
                nNotCheckedPackets = nNotCheckedPackets + nDeleted;
            }
            US8::MessageFormats::Broadcasts::DataPacket packet;
            if (mPacketsToCheckQueue.try_dequeue(packet))
            {
                bool allow{false};
                try
                {
                    if (mFutureDataPacketTester)
                    {
                        allow = mFutureDataPacketTester->allow(packet);
                    }
                    if (allow && mExpiredDataPacketTester)
                    {
                        allow = mExpiredDataPacketTester->allow(packet);
                    }
                    if (allow && mDuplicateDataPacketTester)
                    {
                        allow = mDuplicateDataPacketTester->allow(packet);
                    }
                    if (allow)
                    {
                        mMessagesToPublishQueue.try_enqueue(std::move(packet));
                    }
                    nCheckedPackets = nCheckedPackets + 1;
                }
                catch (const std::exception &e)
                {
                    spdlog::warn("Failed to scrutinize packet because "
                               + std::string {e.what()});
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
                spdlog::info("Checked " 
                    + std::to_string(nCheckedPackets)
                    + " packets in last "
                    + std::to_string(mLogPublishingPerformanceInterval.count())
                    + " seconds. (Failed to check " 
                    + std::to_string(nNotCheckedPackets) + " packets.)");
                nCheckedPackets = 0;
                nNotCheckedPackets = 0;
                lastLogTime = nowSeconds;
            }
        }
        spdlog::debug("Thread leaving checkPackets");
    }
    /// Publishes the good packets
    void publishGoodPackets()
    {
#ifndef NDEBUG
        assert(mPacketPublisher != nullptr);
#endif
        spdlog::debug("Thread entering publishGoodPackets");
        constexpr std::chrono::milliseconds sleepTime{5};
        auto nowMuSeconds
           = std::chrono::time_point_cast<std::chrono::microseconds>
             (std::chrono::high_resolution_clock::now()).time_since_epoch();
        auto lastLogTime
            = std::chrono::duration_cast<std::chrono::seconds> (nowMuSeconds);
        int64_t nSentPackets{0};
        int64_t nNotSentPackets{0};
        while (mKeepRunning)
        {
            if (mMessagesToPublishQueue.size_approx() > MAX_QUEUE_SIZE)
            {
                int nDeleted{0};
                while (mMessagesToPublishQueue.size_approx() > MAX_QUEUE_SIZE)
                {
                    if (!mMessagesToPublishQueue.pop()){break;}
                    nDeleted = nDeleted + 1;
                }
                spdlog::warn("Overfull publisher queue - deleted "
                           + std::to_string(nDeleted) + " packets");
                nNotSentPackets = nNotSentPackets + nDeleted;
            }
            auto packet = mMessagesToPublishQueue.peek();
            if (packet)
            {
                try
                {
                    mPacketPublisher->send(*packet);
                    nSentPackets = nSentPackets + 1;
                }
                catch (const std::exception &e) 
                {
                    spdlog::warn("Failed to send message because "
                               + std::string {e.what()});
                     nNotSentPackets = nNotSentPackets + 1;
                }
                if (!mMessagesToPublishQueue.pop())
                {
                    spdlog::warn("Publisher queue appears to be empty");
                }
/*
                std::string messageType;
                std::string messagePayload;
                try
                {
                    messageType = packet->getMessageType();
                    messagePayload = packet->serialize();
                }
                catch (const std::exception &e) 
                {
                    spdlog::warn("Failed to serialize output message because "
                               + std::string {e.what()});
                }
                if (!mMessagesToPublishQueue.pop())
                {
                    spdlog::warn("Publisher queue appears to be empty");
                }
                try
                {
                    if (messageType.empty())
                    {
                        throw std::runtime_error("Message type is empty");
                    }
                    std::array<zmq::const_buffer, 2> messages{
                       zmq::const_buffer {messageType.data(),
                                          messageType.size()},
                       zmq::const_buffer {messagePayload.data(),
                                          messagePayload.size()}
                    };
                    auto nPartsSent
                        = zmq::send_multipart(mPublisherSocket, messages);
                    if (nPartsSent != 2)
                    {
                        throw std::runtime_error(
                            "Failed to send two-part message");
                    }
                    nSentPackets = nSentPackets + 1;
                }
                catch (const std::exception &e)
                {
                    spdlog::warn("Failed to send message because "
                               + std::string {e.what()});
                    nNotSentPackets = nNotSentPackets + 1;
                }
*/
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
                    + " seconds. (Failed to send " 
                    + std::to_string(nNotSentPackets) + " packets.)");
                nSentPackets = 0;
                nNotSentPackets = 0;
                lastLogTime = nowSeconds;
            }
        }
        spdlog::debug("Thread exiting publishGoodPackets");
    } 
    /// Give main thread something to do until someone says we should quit
    void handleMainThread()
    {   
        spdlog::debug("Main thread entering waiting loop");
        catchSignals();
        {
            while (!mStopRequested)
            {
                if (mInterrupted)
                {
                    spdlog::info("SIGINT/SIGTERM signal received!");
                    mStopRequested = true;
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds {50});
            }
        }
        if (mStopRequested) 
        {
            spdlog::debug("Stop request received.  Terminating proxy...");
            stop();
        }
    }
    /// Handles sigterm and sigint
    static void signalHandler(const int )
    {   
        mInterrupted = true;
    }   
    static void catchSignals()
    {   
        struct sigaction action;
        action.sa_handler = signalHandler;
        action.sa_flags = 0;
        sigemptyset(&action.sa_mask);
        sigaction(SIGINT,  &action, NULL);
        // Kubernetes wants this.  Don't mess with SIGKILL b/c since that is
        // Kubernetes's hammmer.  You basically have 30 seconds to shut
        // down after SIGTERM or the hammer is coming down!
        sigaction(SIGTERM, &action, NULL);
    }   

    Process(const Process &) = delete;
    Process& operator=(const Process &) = delete;
///private:
    //std::thread mSubscriberThread;
    std::thread mTesterThread;
    std::thread mPublisherThread;
    std::unique_ptr<US8::Broadcasts::DataPacket::Subscriber>
        mPacketSubscriber{nullptr};
    std::unique_ptr<US8::Broadcasts::DataPacket::Publisher>
        mPacketPublisher{nullptr};
    std::function<void(US8::MessageFormats::Broadcasts::DataPacket &&packet)>
        mReceivePacketCallback
    {
        std::bind(&::Process::inputPacketsToQueueCallback, this,
                  std::placeholders::_1)
    };
/*
    zmq::context_t mSubscriberContext{1};
    zmq::context_t mPublisherContext{1};
    zmq::socket_t mSubscriberSocket{mSubscriberContext, zmq::socket_type::sub};
    zmq::socket_t mPublisherSocket{mPublisherContext, zmq::socket_type::pub}; 
*/
    std::unique_ptr<USanitizer::TestFutureDataPacket>
        mFutureDataPacketTester;
    std::unique_ptr<USanitizer::TestExpiredDataPacket>
        mExpiredDataPacketTester;
    std::unique_ptr<USanitizer::TestDuplicateDataPacket>
        mDuplicateDataPacketTester;
    moodycamel::ReaderWriterQueue<US8::MessageFormats::Broadcasts::DataPacket>
        mPacketsToCheckQueue{MAX_QUEUE_SIZE};
    moodycamel::ReaderWriterQueue<US8::MessageFormats::Broadcasts::DataPacket>
        mMessagesToPublishQueue{MAX_QUEUE_SIZE};
    std::chrono::seconds mLogPublishingPerformanceInterval{3600};
    std::atomic<bool> mKeepRunning{true};
    bool mStopRequested{false};
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

    std::unique_ptr<::Process> process;
    try
    {   
        process = std::make_unique<::Process> (programOptions);
    }   
    catch (const std::exception &e) 
    {
        spdlog::error("Failed to create proxy process because "
                    + std::string {e.what()});
        return EXIT_FAILURE;
    }

    try
    {   
        process->start();
    }   
    catch (const std::exception &e) 
    {
        spdlog::error("Failed to start proxy process because "
                    + std::string {e.what()});
        return EXIT_FAILURE;
    }   

    process->handleMainThread();
    return EXIT_SUCCESS;
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

::ProgramOptions parseIniFile(const std::filesystem::path &iniFile)
{   
    ::ProgramOptions options;
    if (!std::filesystem::exists(iniFile)){return options;}
    // Parse the initialization file
    boost::property_tree::ptree propertyTree;
    boost::property_tree::ini_parser::read_ini(iniFile, propertyTree);

    // ZeroMQ
    options.inputBroadcastAddress
        = propertyTree.get<std::string> ("ZeroMQ.inputBroadcastAddress",
                                         options.inputBroadcastAddress);
    if (options.inputBroadcastAddress.empty())
    {   
        throw std::invalid_argument("ZeroMQ.inputBroadcastAddress is empty");
    }   
    if (!options.inputBroadcastAddress.starts_with("tcp://"))
    {   
        throw std::invalid_argument(
            "ZeroMQ.inputBroadcastAddress must starts with tcp://");
    }   

    options.outputBroadcastAddress
        = propertyTree.get<std::string> ("ZeroMQ.outputBroadcastAddress",
                                         options.outputBroadcastAddress);
    if (options.outputBroadcastAddress.empty())
    {   
        throw std::invalid_argument("ZeroMQ.outputBroadcastAddress is empty");
    }   
    if (!options.outputBroadcastAddress.starts_with("tcp://"))
    {   
        throw std::invalid_argument(
            "ZeroMQ.outputBroadcastAddress must starts with tcp://");
    }

    // Max future time
    auto maximumFutureTimeInMilliSeconds
        = static_cast<int> (std::chrono::milliseconds
                            {options.maximumFutureTime}.count());
    maximumFutureTimeInMilliSeconds
        = propertyTree.get<int> ("Sanitizer.maximumFutureTimeInMilliSeconds",
                                 maximumFutureTimeInMilliSeconds);
    options.maximumFutureTime
        = std::chrono::milliseconds {maximumFutureTimeInMilliSeconds};

    // Max latency
    auto maximumLatencyInSeconds
        = static_cast<int> (std::chrono::seconds
                            {options.maximumLatency}.count());
    maximumLatencyInSeconds
        = propertyTree.get<int> ("Sanitizer.maximumLatencyInSeconds",
                                 maximumLatencyInSeconds);
    options.maximumLatency
        = std::chrono::seconds {maximumLatencyInSeconds};

    // Circular buffer duration
    auto circularBufferDurationInSeconds
        = static_cast<int> (std::chrono::seconds
                            {options.circularBufferDuration}.count());
    circularBufferDurationInSeconds
        = propertyTree.get<int> ("Sanitizer.circularBufferDurationInSeconds",
                                 circularBufferDurationInSeconds);
    options.circularBufferDuration
        = std::chrono::seconds {circularBufferDurationInSeconds};

    // Logging interval
    auto logBadDataIntervalInSeconds 
        = static_cast<int> (std::chrono::seconds
                            {options.logBadDataInterval}.count());
    logBadDataIntervalInSeconds
        = propertyTree.get<int> ("Sanitizer.logBadDataIntervalInSeconds",
                                 logBadDataIntervalInSeconds);
    options.logBadDataInterval
        = std::chrono::seconds {logBadDataIntervalInSeconds};

    return options;
}
