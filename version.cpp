#include <string>
#include "us8/version.hpp"

using namespace US8;

int Version::getMajor() noexcept
{
    return US8_MAJOR;
}

int Version::getMinor() noexcept
{
    return US8_MINOR;
}

int Version::getPatch() noexcept
{
    return US8_PATCH;
}

bool Version::isAtLeast(const int major, const int minor,
                        const int patch) noexcept
{
    if (US8_MAJOR < major){return false;}
    if (US8_MAJOR > major){return true;}
    if (US8_MINOR < minor){return false;}
    if (US8_MINOR > minor){return true;}
    if (US8_PATCH < patch){return false;}
    return true;
}

std::string Version::getVersion() noexcept
{
    std::string version{US8_VERSION};
    return version;
}

