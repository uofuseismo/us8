#include <iostream>
#include <string>
#include <filesystem>
#include <sstream>
#include <atomic>
#include <chrono>
#include <csignal>
#include <thread>
#include <zmq.hpp>
#include <spdlog/spdlog.h>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include "us8/messaging/zeromq/authentication/zapOptions.hpp"
#include "us8/messaging/zeromq/authentication/service.hpp"

#define FRONTEND_ADDRESS "tcp://127.0.0.1:5550"
#define BACKEND_ADDRESS "tcp://127.0.0.1:5551"


struct ProgramOptions
{
    std::string proxyFrontendAddress{FRONTEND_ADDRESS};
    std::string proxyBackendAddress{BACKEND_ADDRESS};
    int verbosity{3};
    bool helpOnly{false};
};

std::pair<std::string, bool> parseCommandLineOptions(int argc, char *argv[]);
::ProgramOptions parseIniFile(const std::filesystem::path &iniFile);

namespace
{
std::atomic<bool> mInterrupted{false};
}

class Process
{
public:
    enum class ProxyState
    {
        NotRunning,
        Running,
        Paused
    };
public:
    /// @brief Constructor.
    explicit Process(const ProgramOptions &options)
    {
        try
        {
            spdlog::info("Binding frontend proxy socket to " 
                       + options.proxyFrontendAddress);
            US8::Messaging::ZeroMQ::Authentication::Grasslands authenticator;
            US8::Messaging::ZeroMQ::Authentication::GrasslandsServer grasslandsServer;
            grasslandsServer.setSocketOptions(&mFrontendSocket); 
            mFrontendAuthenticator = std::make_unique<US8::Messaging::ZeroMQ::Authentication::Service> (mFrontendContext, std::move(authenticator));
            mFrontendSocket.bind(options.proxyFrontendAddress);
        }
        catch (const std::exception &e)
        {
            auto errorMessage = "Failed to create frontend because "
                              + std::string {e.what()};
            throw std::runtime_error(e.what());
        } 

        spdlog::debug("Creating backend");
        try
        {
            spdlog::info("Binding backend proxy socket to "
                       + options.proxyBackendAddress);
            US8::Messaging::ZeroMQ::Authentication::Grasslands authenticator;
            US8::Messaging::ZeroMQ::Authentication::GrasslandsServer grasslandsServer;
            grasslandsServer.setSocketOptions(&mBackendSocket); 
            mBackendAuthenticator = std::make_unique<US8::Messaging::ZeroMQ::Authentication::Service> (mBackendContext, std::move(authenticator));
            mBackendSocket.set(zmq::sockopt::linger, 0); // Drop pending messages
            mBackendSocket.bind(options.proxyBackendAddress);
        }
        catch (const std::exception &e)
        {
            auto errorMessage = "Failed to create backend because "
                              + std::string {e.what()};
            throw std::runtime_error(e.what());
        }

        spdlog::debug("Creating command/control socket pair...");
        std::ostringstream memoryAddress;
        memoryAddress << static_cast<void const *> (this);
        auto nowMuSec
            = std::chrono::duration_cast<std::chrono::microseconds>
              (std::chrono::system_clock::now().time_since_epoch()).count();
        mControlAddress = "inproc://"
                        + std::to_string(nowMuSec)
                        + "_" + memoryAddress.str()
                        + "_xpubsub_proxy_control";
          
        try
        {
            mControlSocket.bind(mControlAddress);
        }
        catch (const std::exception &e)
        {
            auto errorMessage = "Failed to bind to control socket failed with "
                              + std::string {e.what()};
            throw std::runtime_error(errorMessage);
        }

        try
        {
            mCommandSocket.set(zmq::sockopt::linger, 0);
            mCommandSocket.connect(mControlAddress);
        }
        catch (const std::exception &e)
        {
            auto errorMessage 
                = "Failed to connect to control socket; failed with "
                + std::string {e.what()};
            throw std::runtime_error(errorMessage);
        }
    }
    /// @brief Destructor
    ~Process()
    {
        stop();
    }
    /// @brief Starts the proxy.
    void start()
    {
        if (mFrontendAuthenticator)
        {
            spdlog::info("Starting the frontend authenticator");
            mFrontendAuthenticator->start();
        }
        if (mBackendAuthenticator)
        {
            spdlog::info("Starting the backend authenticator");
            mBackendAuthenticator->start();
        }
        spdlog::info("Starting the proxy");
        mProxyThread = std::thread(&::Process::proxyRun, this);
        mProxyState = ProxyState::Running;
    }
    /// Function for thread
    void proxyRun()
    {
        zmq::proxy_steerable(mFrontendSocket,
                             mBackendSocket,
                             zmq::socket_ref(),
                             mControlSocket);
    }
    /// @brief Resumes the proxy after a pause.
    void resume()
    {
        if (mProxyState == ProxyState::Running)
        {
            spdlog::info("Resuming the proxy");
            try
            {
                mCommandSocket.send(zmq::str_buffer("RESUME"),
                                    zmq::send_flags::none);
                mProxyState = ProxyState::Paused;
            }
            catch (const std::exception &e)
            {
                spdlog::error("Failed to resume the proxy because "
                            + std::string {e.what()});
            }
        }
    }
    /// @brief Allows the main thread to pause the proxy.
    void pause()
    {
        if (mProxyState == ProxyState::Running)
        {
            spdlog::info("Pausing the proxy");
            try
            {
                mCommandSocket.send(zmq::str_buffer("PAUSE"),
                                    zmq::send_flags::none);
                mProxyState = ProxyState::Paused;
            }
            catch (const std::exception &e)
            {
                spdlog::error("Failed to pause the proxy because "
                            + std::string {e.what()});
            }
        }
    } 
    /// @brief Allows the main thread to stop the proxy.
    void stop()
    {
        if (mFrontendAuthenticator){mFrontendAuthenticator->stop();}
        if (mBackendAuthenticator){mBackendAuthenticator->stop();}
        if (mProxyState == ProxyState::Running || 
            mProxyState == ProxyState::Paused)
        {
            spdlog::info("Terminating the proxy");
            try
            {
                mCommandSocket.send(zmq::str_buffer("TERMINATE"),
                                    zmq::send_flags::none);
                mProxyState = ProxyState::NotRunning;
            }
            catch (const std::exception &e)
            {
                spdlog::error("Failed to terminate proxy because "
                            + std::string {e.what()});
            }
        }
        if (mProxyThread.joinable()){mProxyThread.join();}
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
/*
                std::unique_lock<std::mutex> lock(mStopContext);
                mStopCondition.wait_for(lock,
                                        std::chrono::milliseconds {100},
                                        [this]
                                        {
                                              return mStopRequested;
                                        });
                lock.unlock();
*/
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
//private:
    std::thread mProxyThread;
    std::string mControlAddress;
    std::shared_ptr<zmq::context_t> mFrontendContext{std::make_shared<zmq::context_t> (1)};
    std::shared_ptr<zmq::context_t> mBackendContext{std::make_shared<zmq::context_t> (1)};
    std::unique_ptr<US8::Messaging::ZeroMQ::Authentication::Service> mFrontendAuthenticator{nullptr};
    std::unique_ptr<US8::Messaging::ZeroMQ::Authentication::Service> mBackendAuthenticator{nullptr};
    zmq::context_t mControlContext{1};
    zmq::socket_t mFrontendSocket{*mFrontendContext, zmq::socket_type::xsub}; 
    zmq::socket_t mBackendSocket{*mBackendContext,  zmq::socket_type::xpub};
    zmq::socket_t mControlSocket{mControlContext, zmq::socket_type::rep};
    zmq::socket_t mCommandSocket{mControlContext, zmq::socket_type::req};
    std::atomic<ProxyState> mProxyState{ProxyState::NotRunning};
    bool mStopRequested{false};
};

int main(int argc, char *argv[])
{
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
/*
    std::this_thread::sleep_for(std::chrono::seconds {4});

    process->stop(); 
*/
    return EXIT_SUCCESS;
}

/// Parses the command line options
std::pair<std::string, bool> parseCommandLineOptions(int argc, char *argv[])
{
    std::string iniFile;
    boost::program_options::options_description desc(
R"""(
The dataPacketBroadcastProxy is middleware to which programs can forward 
data from utilities like SEEDLink to a cluster data broadcast.

Example usage:
    dataPacketBroadcastProxy --ini=iniFile

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
/*
    desc.add_options()
        ("help",    "Produces this help message")
        ("frontend", boost::program_options::value<std::string> ()->default_value(FRONTEND_ADDRESS),
                     "The address to which data publishers will connect")
        ("backend",  boost::program_options::value<std::string> ()->default_value(BACKEND_ADDRESS),
                     "The address to which data subscribers will connect");
    boost::program_options::variables_map vm;
    boost::program_options::store(
        boost::program_options::parse_command_line(argc, argv, desc), vm);
    boost::program_options::notify(vm);
    if (vm.count("help"))
    {   
        std::cout << desc << std::endl;
        result.helpOnly = true;
        return result;
    }   
    if (vm.count("frontend"))
    {
        auto address = vm["frontend"].as<std::string>();
        if (address.empty())
        {
            throw std::invalid_argument("Frontend address is empty");
        }
        if (!address.starts_with("tcp://"))
        {
            address = "tcp://" + address;      
        }
        result.proxyFrontendAddress = address;
    }   
    if (vm.count("backend"))
    {
        auto address = vm["backend"].as<std::string>();
        if (address.empty())
        {
            throw std::invalid_argument("Backend address is empty");
        }
        if (!address.starts_with("tcp://"))
        {
            address = "tcp://" + address;
        }
        result.proxyBackendAddress = address;
    }
    if (result.proxyFrontendAddress == result.proxyBackendAddress)
    {
        throw std::runtime_error("Frontend address "
                               + result.proxyFrontendAddress
                               + " cannot be the same as the backend address "
                               + result.proxyBackendAddress);
    }
    return result; 
*/
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

    options.proxyBackendAddress
        = propertyTree.get<std::string> ("ZeroMQ.proxyBackendAddress",
                                         options.proxyBackendAddress);
    if (options.proxyBackendAddress.empty())
    {
        throw std::invalid_argument("ZeroMQ.proxyBackendAddress is empty");
    }
    if (!options.proxyBackendAddress.starts_with("tcp://"))
    {
        throw std::invalid_argument(
            "ZeroMQ.proxyBackendAddresss must starts with tcp://");
    }

    return options;
}
