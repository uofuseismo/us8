#include <iostream>
#include <chrono>
#include <csignal>
#include <spdlog/spdlog.h>
#include <thread>
#include <filesystem>
#include <cerrno>
#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <readerwriterqueue.h>
#include <opentelemetry/nostd/shared_ptr.h>
#include <opentelemetry/metrics/meter.h>
#include <opentelemetry/metrics/meter_provider.h>
#include <opentelemetry/metrics/provider.h>
#include <opentelemetry/exporters/ostream/metric_exporter_factory.h>
#include <opentelemetry/sdk/metrics/meter_context.h>
#include <opentelemetry/sdk/metrics/meter_context_factory.h>
#include <opentelemetry/sdk/metrics/meter_provider.h>
#include <opentelemetry/sdk/metrics/meter_provider_factory.h>
#include <opentelemetry/sdk/metrics/provider.h>
#include <opentelemetry/sdk/metrics/export/periodic_exporting_metric_reader_factory.h>
#include <opentelemetry/sdk/metrics/export/periodic_exporting_metric_reader_options.h>
#include <opentelemetry/sdk/metrics/view/instrument_selector_factory.h>
#include <opentelemetry/sdk/metrics/view/meter_selector_factory.h>
#include "client.hpp"
#include "clientOptions.hpp"
#include "streamSelector.hpp"
#include "us8/broadcasts/dataPacket/publisher.hpp"
#include "us8/broadcasts/dataPacket/publisherOptions.hpp"
#include "us8/messageFormats/broadcasts/dataPacket.hpp"

#define APPLICATION_NAME "seedlink_data_packet_broadcaster"
#define PROXY_FRONTEND_ADDRESS "tcp://127.0.0.1:5550"
#define MAX_QUEUE_SIZE 1024
#define OTEL_VERSION "1.2.0"
#define OTEL_SCHEMA "https://opentelemetry.io/schemas/1.2.0"

namespace
{
std::atomic<bool> mInterrupted{false};
}


struct ProgramOptions
{
    US8::Broadcasts::DataPacket::SEEDLink::ClientOptions seedLinkClientOptions;
    std::string applicationName{APPLICATION_NAME};
    std::string openTelemetrySchema{OTEL_SCHEMA};
    std::string openTelemetryVersion{OTEL_VERSION};
    std::string proxyFrontendAddress{PROXY_FRONTEND_ADDRESS};
    // Maximum time before a send operation returns with EAGAIN
    // -1 waits forever whereas 0 returns immediately.
    std::chrono::milliseconds sendTimeOut{1000}; // 1s is enough
    std::chrono::seconds oldestPacket{-1};
    std::chrono::seconds logPublishingPerformanceInterval{600}; // Every 10 minutes 
    std::chrono::milliseconds openTelemetryExportInterval{60000}; // 1 second
    std::chrono::milliseconds openTelemetryTimeOut{500};
    int sendHighWaterMark{4096};
    int verbosity{3};
    bool preventFuturePackets{true};
};

::ProgramOptions parseIniFile(const std::filesystem::path &iniFile);
std::pair<std::string, bool> parseCommandLineOptions(int argc, char *argv[]);

class Process
{
public:
    Process() = delete;
    explicit Process(const ProgramOptions &options) :
        mOptions(options)
    {
        // Create the exporter
        auto exporter 
            = opentelemetry::exporter::metrics::OStreamMetricExporterFactory::Create();

        // Create the provider and reader
        opentelemetry::sdk::metrics::PeriodicExportingMetricReaderOptions meterOptions;
//        meterOptions.export_interval_millis = options.openTelemetryExportInterval;
        meterOptions.export_timeout_millis = options.openTelemetryTimeOut;
        auto reader
            = opentelemetry::sdk::metrics::PeriodicExportingMetricReaderFactory
                ::Create(std::move(exporter), meterOptions); 

        auto context
            = opentelemetry::sdk::metrics::MeterContextFactory::Create();
        context->AddMetricReader(std::move(reader));
        auto provider
            = opentelemetry::sdk::metrics::MeterProviderFactory::Create(std::move(context));

/*
        auto gaugeName = options.applicationName
                       + "-propagated_packets_gauge";
        auto gaugeUnit = "packets/minute";

        auto instrumentSelector
            = opentelemetry::sdk::metrics::InstrumentSelectorFactory::Create(
                 opentelemetry::sdk::metrics::InstrumentType::kGauge, 
                 gaugeName,
                 gaugeUnit);

        auto meterSelector 
            = opentelemetry::sdk::metrics::MeterSelectorFactory::Create(
                 options.applicationName,
                 options.openTelemetryVersion,
                 options.openTelemetrySchema);
        auto sumView = opentelemetry::sdk::metrics::ViewFactory::Create(
                          options.applicationName,
                          opentelemetry::sdk:metrics::AggregationType::kSum);

        provider->AddView(std::move(instrumentSelector),
                          std::move(meterSelector),
                          std::move(sumView));
*/

        std::shared_ptr<opentelemetry::metrics::MeterProvider> apiProvider(std::move(provider));
        opentelemetry::sdk::metrics::Provider::SetMeterProvider(apiProvider);
                             

        mLogPublishingPerformanceInterval
            = options.logPublishingPerformanceInterval;
        // Initialize ZMQ publisher
        try
        {
            US8::Broadcasts::DataPacket::PublisherOptions publisherOptions{
                options.proxyFrontendAddress};
            publisherOptions.setHighWaterMark(options.sendHighWaterMark);
            publisherOptions.setTimeOut(options.sendTimeOut);
            
            mPacketPublisher
                = std::make_unique<US8::Broadcasts::DataPacket::Publisher>
                  (publisherOptions);
        }
        catch (const std::exception &e)
        {
            auto errorMessage = "Failed to initialize publisher socket because "
                              + std::string {e.what()};
            throw std::runtime_error(errorMessage);
        }
        // Initialize SEEDLink client
        try
        {
            mSEEDLinkClient
                = std::make_unique<US8::Broadcasts::DataPacket::SEEDLink::Client>
                     (mAddPacketsFromAcquisitionCallback,
                      options.seedLinkClientOptions);
        }
        catch (const std::exception &e)
        {
            auto errorMessage = "Failed to create SEEDLink client because "
                              + std::string {e.what()};
            throw std::runtime_error(errorMessage);
        }
    }
    /// Destructor
    ~Process()
    {
        stop();
        cleanupMetrics();
    }
    /// Cleanup metrics
    void cleanupMetrics()
    {
        std::shared_ptr<opentelemetry::metrics::MeterProvider> noProvider;
        opentelemetry::sdk::metrics::Provider::SetMeterProvider(noProvider);
    }
    /// Start the service
    void start()
    {
        stop();
        mKeepRunning = true;
        mPublisherThread = std::thread(&::Process::sendPacketsViaZeroMQ, this);
#ifndef NDEBUG
        assert(mSEEDLinkClient);
#endif
        mSEEDLinkClient->start(); 
    }
    /// Stops the threads
    void stop()
    {
        mKeepRunning = false; 
        if (mPublisherThread.joinable()){mPublisherThread.join();}
    }
    /// Sends packets to the proxy via ZeroMQ
    void sendPacketsViaZeroMQ()
    {
        // Get my monitoring stuff
        auto provider = opentelemetry::metrics::Provider::GetMeterProvider();

        opentelemetry::nostd::shared_ptr<opentelemetry::metrics::Meter>
            meter = provider->GetMeter(mOptions.applicationName,
                                       mOptions.openTelemetryVersion);
        auto propagatedGaugeName = mOptions.applicationName
                                 + "-propagated_packets_gauge";
        auto notPropagatedGaugeName = mOptions.applicationName
                                    + "-skipped_packets_gauge";
        auto publishedPacketsGauge
            = meter->CreateInt64Gauge(
                 propagatedGaugeName,
                 "Number of packets propagated from SEEDLink to ZeroMQ in last minute",
                 "packets/minute");
        auto notPublishedPacketsGauge
            = meter->CreateInt64Gauge(
                 notPropagatedGaugeName,
                 "Number of packets not propagated from SEEDLink to ZeroMQ in last minute",
                 "packets/minute");
        auto context = opentelemetry::context::Context{};

#ifndef NDEBUG
        assert(mPacketPublisher != nullptr);
#endif
        spdlog::info("Thread entering publisher");
        std::chrono::milliseconds sleepTime{5};
        bool logPublishingPerformance
            = mLogPublishingPerformanceInterval.count() > 0 ? true : false;
        auto nowMuSeconds
            = std::chrono::time_point_cast<std::chrono::microseconds>
              (std::chrono::high_resolution_clock::now()).time_since_epoch();
        auto lastLogTime
            = std::chrono::duration_cast<std::chrono::seconds> (nowMuSeconds);
        auto nextSendMetricTime
            = std::chrono::duration_cast<std::chrono::seconds> (nowMuSeconds)
            + std::chrono::seconds {60};
        int64_t nSentPackets{0};
        int64_t nNotSentPackets{0};
        uint64_t nSentPacketsInLastMinute{0};
        uint64_t nNotSentPacketsInLastMinute{0};
        while (mKeepRunning)
        {
            if (mQueue.size_approx() > MAX_QUEUE_SIZE)
            {
                int nDeleted{0};
                while (mQueue.size_approx() > MAX_QUEUE_SIZE)
                {
                    if (!mQueue.pop()){break;}
                    nDeleted = nDeleted + 1;
                } 
                spdlog::warn("Overfull queue - deleted "
                           + std::to_string(nDeleted) + " packets");
                nNotSentPackets = nNotSentPackets + nDeleted;
                nNotSentPacketsInLastMinute
                    = nNotSentPacketsInLastMinute + nDeleted;
            }
            // Got a packet?
            auto packet = mQueue.peek();
            if (packet)
            {
                try
                {
                    mPacketPublisher->send(*packet);
                    nSentPackets = nSentPackets + 1;
                    nSentPacketsInLastMinute = nSentPacketsInLastMinute + 1;
                }
                catch (const std::exception &e) 
                {   
                    spdlog::warn("Failed to send message because "
                               + std::string {e.what()});
                     nNotSentPackets = nNotSentPackets + 1;
                }   
                if (!mQueue.pop()){spdlog::warn("Queue appears to be empty");}
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
            if (logPublishingPerformance &&
                nowSeconds >= lastLogTime + mLogPublishingPerformanceInterval)
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
            if (nowSeconds >= nextSendMetricTime)
            {
spdlog::info("Yes");
                try
                {
                    publishedPacketsGauge->Record(nSentPacketsInLastMinute, context);
                    notPublishedPacketsGauge->Record(nNotSentPacketsInLastMinute, context);
                }
                catch (const std::exception &e)
                {
                    spdlog::warn("Failed to publish metrics because "
                               + std::string {e.what()});
                }
                nSentPacketsInLastMinute = 0;
                nNotSentPacketsInLastMinute = 0;
                nextSendMetricTime
                    = nowSeconds + std::chrono::seconds {60};
            }
        }
        spdlog::info("Thread exiting publisher");
    }
    void addPacketsFromAcquisitionCallback(
        US8::MessageFormats::Broadcasts::DataPacket &&packet)
    {
        // Check the packet 
        if (!packet.haveNetwork())
        {
            throw std::invalid_argument("Packet does not have network code");
        }
        if (!packet.haveStation())
        {
            throw std::invalid_argument("Packet does nto have station name");
        }
        if (!packet.haveChannel())
        {
            throw std::invalid_argument("Packet does not have channel code");
        }
        if (!packet.haveSamplingRate())
        {
            throw std::invalid_argument("Packet does not have sampling rate");
        }
        if (packet.getNumberOfSamples() < 1)
        {
            throw std::invalid_argument("Packet has no data");
        }
#ifndef NDEBUG
        assert(packet.getDataType() !=
               US8::MessageFormats::Broadcasts::DataPacket::DataType::Unknown);
#else
        if (packet.getDataType() ==
            US8::MessageFormats::Broadcasts::DataPacket::DataType::Unknown)
        {
            throw std::runtime_error("Data type not correctly set");
        }
#endif
        // Hand it off to the publisher socket
        if (!mQueue.try_enqueue(std::move(packet)))
        {
            spdlog::warn("Failed to add packet to queue");
        }
    }
    /// Place for the main thread to sleep until someone wakes it up.
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
        sigaction(SIGTERM, &action, NULL);
    }   
///private:
    ::ProgramOptions mOptions;
    std::thread mAcquisitionThread;
    std::thread mPublisherThread;
    std::unique_ptr<US8::Broadcasts::DataPacket::Publisher>
        mPacketPublisher{nullptr};
    moodycamel::ReaderWriterQueue<US8::MessageFormats::Broadcasts::DataPacket>
        mQueue{MAX_QUEUE_SIZE};
    std::unique_ptr<US8::Broadcasts::DataPacket::SEEDLink::Client>
        mSEEDLinkClient{nullptr};
    std::unique_ptr<opentelemetry::sdk::metrics::PushMetricExporter>
        mMetricsExporter{nullptr};
    std::function<void(US8::MessageFormats::Broadcasts::DataPacket &&packet)>
        mAddPacketsFromAcquisitionCallback
    {
        std::bind(&::Process::addPacketsFromAcquisitionCallback, this,
                  std::placeholders::_1)
    };
    std::chrono::seconds mLogPublishingPerformanceInterval{600};
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
        programOptions = ::parseIniFile(iniFile);
    }
    catch (const std::exception &e)
    {
        spdlog::error(e.what());
        return EXIT_FAILURE;
    }

    // Verbosity
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
The seedLinkDataPacketBroadcastPublisher reads packets from a SEEDLink 
server then propagates them to a US8 data packet broadcasts proxy.
Example usage is

    seedLinkDataPacketBroadcastPublisher --ini=loader.ini

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

[[nodiscard]] US8::Broadcasts::DataPacket::SEEDLink::ClientOptions
getSEEDLinkOptions(const boost::property_tree::ptree &propertyTree,
                   const std::string &clientName)
{
    namespace USL = US8::Broadcasts::DataPacket::SEEDLink;
    USL::ClientOptions clientOptions;
    auto address = propertyTree.get<std::string> (clientName + ".address");
    auto port = propertyTree.get<uint16_t> (clientName + ".port", 18000);
    clientOptions.setAddress(address);
    clientOptions.setPort(port);
    for (int iSelector = 1; iSelector <= 32768; ++iSelector)
    {
        std::string selectorName{clientName
                               + ".data_selector_"
                               + std::to_string(iSelector)};
        auto selectorString
            = propertyTree.get_optional<std::string> (selectorName);
        if (selectorString)
        {
            std::vector<std::string> splitSelectors;
            boost::split(splitSelectors, *selectorString,
                         boost::is_any_of(",|"));
            // A selector string can look like:
            // UU.FORK.HH?.01 | UU.CTU.EN?.01 | ....
            for (const auto &thisSplitSelector : splitSelectors)
            {
                std::vector<std::string> thisSelector; 
                auto splitSelector = thisSplitSelector;
                boost::algorithm::trim(splitSelector);

                boost::split(thisSelector, splitSelector,
                             boost::is_any_of(" \t"));
                USL::StreamSelector selector;
                if (splitSelector.empty())
                {
                    throw std::invalid_argument("Empty selector");
                }
                // Require a network
                boost::algorithm::trim(thisSelector.at(0));
                selector.setNetwork(thisSelector.at(0));
                // Add a station?
                if (splitSelector.size() > 1)
                {
                    boost::algorithm::trim(thisSelector.at(1));
                    selector.setStation(thisSelector.at(1));
                }
                // Add channel + location code + data type
                std::string channel{"*"};
                std::string locationCode{"??"};
                if (splitSelector.size() > 2)
                {
                    boost::algorithm::trim(thisSelector.at(2));
                    channel = thisSelector.at(2);
                }
                if (splitSelector.size() > 3)
                {
                    boost::algorithm::trim(thisSelector.at(3));
                    locationCode = thisSelector.at(3);
                }
                // Data type
                auto dataType = USL::StreamSelector::Type::All;
                if (splitSelector.size() > 4)
                {
                    boost::algorithm::trim(thisSelector.at(4));
                    if (thisSelector.at(4) == "D")
                    {
                        dataType = USL::StreamSelector::Type::Data;
                    }
                    else if (thisSelector.at(4) == "A")
                    {
                        dataType = USL::StreamSelector::Type::All; 
                    }
                    // TODO other data types
                }
                selector.setSelector(channel, locationCode, dataType);
                clientOptions.addStreamSelector(selector);
            } // Loop on selectors
        } // End check on selector string
    } // Loop on selectors
    return clientOptions;
}

::ProgramOptions parseIniFile(const std::filesystem::path &iniFile)
{   
    ::ProgramOptions options;
    if (!std::filesystem::exists(iniFile)){return options;}
    // Parse the initialization file
    boost::property_tree::ptree propertyTree;
    boost::property_tree::ini_parser::read_ini(iniFile, propertyTree);

    // ZeroMQ
    options.proxyFrontendAddress
        = propertyTree.get<std::string> ("ZeroMQ.proxyFrontendAddress",
                                         options.proxyFrontendAddress);
    if (options.proxyFrontendAddress.empty())
    {
        throw std::invalid_argument("ZeroMQ.proxyFrontendAddress is empty");
    }
    if (!options.proxyFrontendAddress.starts_with("tcp://"))
    {
        throw std::invalid_argument(
            "ZeroMQ.proxyFrontendAddresss must starts with tcp://");
    }
    options.sendHighWaterMark
        = propertyTree.get<int> ("ZeroMQ.sendHighWaterMark",
                                 options.sendHighWaterMark);
    if (options.sendHighWaterMark < 0)
    {
        throw std::invalid_argument(
            "ZeroMQ.sendHighWaterMark cannot be negative"); 
    }
    auto sendTimeOut = static_cast<int> (options.sendTimeOut.count());
    sendTimeOut
        = propertyTree.get<int> ("ZeroMQ.sendTimeOutInMilliSeconds",
                                 sendTimeOut);
    // 0 is immediate return and -1 is wait until a new message is received
    if (sendTimeOut < 0){sendTimeOut = -1;}
    options.sendTimeOut = std::chrono::milliseconds {sendTimeOut}; 
                                         
    // SEEDLink properties
    if (propertyTree.get_optional<std::string> ("SEEDLink.address"))
    {
        options.seedLinkClientOptions 
             = ::getSEEDLinkOptions(propertyTree, "SEEDLink");
    }

    auto logPublishingPerformanceIntervalInSeconds
        = static_cast<int> (std::chrono::seconds
                            {options.logPublishingPerformanceInterval}.count());
    logPublishingPerformanceIntervalInSeconds
        = propertyTree.get<int> (
          "General.logPublishingPerformanceIntervalInSeconds",
          logPublishingPerformanceIntervalInSeconds);
    options.logPublishingPerformanceInterval
        = std::chrono::seconds {logPublishingPerformanceIntervalInSeconds};
    return options;
}
