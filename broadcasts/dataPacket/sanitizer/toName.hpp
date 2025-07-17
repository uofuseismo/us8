#ifndef TO_NAME_HPP
#define TO_NAME_HPP
#include <string>
#include "us8/messageFormats/broadcasts/dataPacket.hpp"

namespace
{

[[nodiscard]] std::string 
toName(const US8::MessageFormats::Broadcasts::DataPacket &packet)
{
    auto network = packet.getNetwork();
    auto station = packet.getStation(); 
    auto channel = packet.getChannel();
    std::string locationCode;
    try 
    {   
        locationCode = packet.getLocationCode();
    }   
    catch (...)
    {   
    }   
    auto name = network + "."  + station + "." + channel;
    if (!locationCode.empty())
    {   
        name = name + "." + locationCode;
    }   
    return name;

}

}
#endif
