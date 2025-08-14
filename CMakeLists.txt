cmake_minimum_required(VERSION 3.28)
project(us8 VERSION 0.1.0 LANGUAGES CXX)
enable_testing()

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)
option(BUILD_FOR_DEBIAN  "Build for Debian application container" ON)
include(GenerateExportHeader)
include(FetchContent)

FetchContent_Declare(
  readerwriterqueue
  GIT_REPOSITORY    https://github.com/cameron314/readerwriterqueue
  GIT_TAG           8b2176698e9bdaba653cdc20c32b54737a934b47
  EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(readerwriterqueue)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(Boost_USE_STATIC_LIBS ON)
find_package(spdlog REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)
find_package(ZeroMQ "4.3" REQUIRED)
find_package(cppzmq "4" REQUIRED)
find_package(Boost COMPONENTS program_options REQUIRED)
find_package(SEEDLink)
find_package(MiniSEED)
find_package(Catch2 3)
find_package(Threads REQUIRED)
#find_program(Podman_EXECUTABLE podman REQUIRED)

set(BINARIES "")
set(CLIENT_LIBRARY_SRC 
    version.cpp
    messageFormats/message.cpp
    messageFormats/broadcasts/dataPacket.cpp
    broadcasts/dataPacket/publisher.cpp
    broadcasts/dataPacket/publisherOptions.cpp
    broadcasts/dataPacket/subscriberOptions.cpp
    broadcasts/dataPacket/subscriber.cpp) 
configure_file(${CMAKE_SOURCE_DIR}/version.hpp.in
               ${CMAKE_SOURCE_DIR}/include/us8/version.hpp)
add_library(us8client ${CLIENT_LIBRARY_SRC})
target_sources(us8client
               PUBLIC
               FILE_SET HEADERS
               BASE_DIRS
                  ${CMAKE_CURRENT_SOURCE_DIR}
               FILES 
                  include/us8/messageFormats/message.hpp
                  include/us8/messageFormats/broadcasts/dataPacket.hpp
               )
set_target_properties(us8client PROPERTIES
                      CXX_STANDARD 20
                      CXX_STANDARD_REQUIRED YES
                      CXX_EXTENSIONS NO)
target_include_directories(us8client
                           PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
                           PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>)
target_link_libraries(us8client
                      PRIVATE cppzmq-static
                              nlohmann_json::nlohmann_json
                              spdlog::spdlog_header_only
                              -static-libstdc++)

set(MESSAGING_LIBRARY_SRC
    messaging/authentication/authenticator.cpp
    messaging/authentication/credential/userNameAndPassword.cpp
    messaging/authentication/credential/keyPair.cpp
    messaging/zeromq/authentication/service.cpp
    messaging/zeromq/authentication/grasslands.cpp
    messaging/zeromq/authentication/zapOptions.cpp)
set(WEBSERVER_LIBRARY_SRC
    services/webServer/exception.cpp
    services/webServer/listener.cpp)
add_library(us8messaging ${MESSAGING_LIBRARY_SRC} ${WEBSERVER_LIBRARY_SRC})
target_sources(us8messaging
               PUBLIC
               FILE_SET HEADERS
               BASE_DIRS
                  ${CMAKE_CURRENT_SOURCE_DIR}
               FILES
                  include/us8/messaging/authentication/exceptions.hpp
                  include/us8/messaging/authentication/authenticator.hpp
                  include/us8/messaging/zeromq/authentication/service.hpp
                  include/us8/messaging/zeromq/authentication/zapOptions.hpp
                  include/us8/messaging/zeromq/authentication/grasslands.hpp)
set_target_properties(us8messaging PROPERTIES
                      CXX_STANDARD 20
                      CXX_STANDARD_REQUIRED YES
                      CXX_EXTENSIONS NO)
target_include_directories(us8messaging
                           PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
                           PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>)
target_link_libraries(us8messaging
                      PRIVATE spdlog::spdlog_header_only cppzmq-static Threads::Threads -static-libstdc++)

add_executable(dataPacketBroadcastProxy 
               broadcasts/dataPacket/proxy.cpp)
set_target_properties(dataPacketBroadcastProxy PROPERTIES
                      CXX_STANDARD 20
                      CXX_STANDARD_REQUIRED YES 
                      CXX_EXTENSIONS NO) 
target_include_directories(dataPacketBroadcastProxy
                           PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/lib>
                           PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>)
target_link_libraries(dataPacketBroadcastProxy
                      PRIVATE us8messaging us8client spdlog::spdlog_header_only Boost::program_options
                              cppzmq-static Threads::Threads -static-libstdc++)
list(APPEND BINARIES dataPacketBroadcastProxy)

add_executable(dataPacketSanitizer
               broadcasts/dataPacket/sanitizer/sanitizer.cpp
               broadcasts/dataPacket/sanitizer/testFutureDataPacket.cpp
               broadcasts/dataPacket/sanitizer/testExpiredDataPacket.cpp
               broadcasts/dataPacket/sanitizer/testDuplicateDataPacket.cpp)
set_target_properties(dataPacketSanitizer PROPERTIES
                      CXX_STANDARD 20
                      CXX_STANDARD_REQUIRED YES
                      CXX_EXTENSIONS NO)
target_include_directories(dataPacketSanitizer
                           PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/lib>
                           PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>)
target_link_libraries(dataPacketSanitizer
                      PRIVATE us8messaging us8client spdlog::spdlog_header_only Boost::program_options
                              readerwriterqueue Threads::Threads -static-libstdc++)
list(APPEND BINARIES dataPacketSanitizer)


if (${SEEDLink_FOUND} AND ${MiniSEED_FOUND})
   add_executable(seedLinkDataPacketBroadcastPublisher
                  broadcasts/dataPacket/importClient.cpp
                  broadcasts/dataPacket/seedLink/publisher.cpp
                  broadcasts/dataPacket/seedLink/streamSelector.cpp
                  broadcasts/dataPacket/seedLink/clientOptions.cpp
                  broadcasts/dataPacket/seedLink/client.cpp)
   set_target_properties(seedLinkDataPacketBroadcastPublisher PROPERTIES
                         CXX_STANDARD 20
                         CXX_STANDARD_REQUIRED YES 
                         CXX_EXTENSIONS NO) 
   target_compile_definitions(seedLinkDataPacketBroadcastPublisher
                              PRIVATE USE_MS_VERSION_315)
   target_include_directories(seedLinkDataPacketBroadcastPublisher
                              PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/lib:${MINISEED_INCLUDE_DIR}:${SEEDLINK_INCLUDE_DIR}> 
                              PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>)
   target_link_libraries(seedLinkDataPacketBroadcastPublisher
                         PRIVATE us8client us8messaging spdlog::spdlog_header_only Boost::program_options
                                 ${SEEDLINK_LIBRARY} ${MINISEED_LIBRARY}
                                 readerwriterqueue Threads::Threads -static-libstdc++)
   list(APPEND BINARIES seedLinkDataPacketBroadcastPublisher)
endif()



##########################################################################################
#                                      Installation                                      #
##########################################################################################
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
configure_package_config_file(
    cmake/${PROJECT_NAME}Config.cmake.in "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    VERSION "${version}"
    COMPATIBILITY AnyNewerVersion
)

install(TARGETS us8client us8messaging
        EXPORT ${PROJECT_NAME}-targets
        FILE_SET HEADERS
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        COMPONENT libraries)
install(TARGETS ${BINARIES}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT applications)
export(EXPORT ${PROJECT_NAME}-targets
       FILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Targets.cmake")
install(EXPORT ${PROJECT_NAME}-targets
        FILE ${PROJECT_NAME}Targets.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})
install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})

##########################################################################################
#                                        Containers                                      #
##########################################################################################

if (${BUILD_FOR_DEBIAN})
   configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scripts/Dockerfile.deb.in
                  ${CMAKE_CURRENT_BINARY_DIR}/Dockerfile.deb @ONLY)
endif()


##########################################################################################
#                                     CPACK Packaging                                    #
##########################################################################################
include(CPackComponent)
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VENDOR "UUSS")
set(CPACK_PACKAGE_CONTACT "ben.baker@utah.edu")
set(CPACK_PACKAGE_LICENSE "MIT")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "UUSS Seismic System for Kubernetes (US8) is a collection of microservices to be managed by Kubernetes for performing seismic monitoring activities.")
set(CPACK_PACKAGE_INSTALL_DIRECTORY ${CPACK_PACKAGE_NAME})
set(CPACK_VERBATIM_VARIABLES YES)
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
cpack_add_component(libraries
                    DISPLAY_NAME "us8 common libraries"
                    DEPENDS ${MINISEED_LIBRARY})
cpack_add_component(headers
                    DISPLAY_NAME "us8 library header files"
                    DEPENDS libraries)
cpack_add_component(applications
                    DISPLAY_NAME "us8 applications"
                    DEPENDS libraries)
set(CPACK_GENERATOR ZIP TGZ)
if (WIN32)
   list(APPEND CPACK_GENERATOR WIX)
elseif (APPLE)
   list(APPEND CPACK_GENERATOR productbuild)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
   find_program(RPMBUILD_PROGRAM rpmbuild)
   if (RPMBUILD_PROGRAM)
      list(APPEND CPACK_GENERATOR RPM)
      #set(CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES abc >= 123")
   endif()
   if (${BUILD_FOR_DEBIAN})
      find_program(DPKG_PROGRAM dpkg REQUIRED)
   else()
      find_program(DPKG_PROGRAM dpkg)
   endif()
   if (DPKG_PROGRAM)
      list(APPEND CPACK_GENERATOR DEB)
      set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}")# libsodium (>= 1.0.20) libzmq (>=4.3.0)")
      set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
   endif()
else()
   message("Unknown operating system")
endif()
set(CPACK_SOURCE_IGNORE_FILES
  /\\.git/
  \\.swp
  \\.orig
  /CMakeLists\\.txt\\.user
  /private/
)
include(CPack) # Put this last!

