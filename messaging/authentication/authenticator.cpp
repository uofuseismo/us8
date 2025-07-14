#include "us8/messaging/authentication/authenticator.hpp"
#include "us8/messaging/authentication/credential/keyPair.hpp"
#include "us8/messaging/authentication/credential/userNameAndPassword.hpp"

using namespace US8::Messaging::Authentication;

IAuthenticator::~IAuthenticator() = default;

std::string IAuthenticator::operator()(
    const Credential::KeyPair &keys) const
{
    return authenticate(keys);
}

std::string IAuthenticator::operator()(
    const Credential::UserNameAndPassword &userNameAndPassword) const
{
    return authenticate(userNameAndPassword);
}
