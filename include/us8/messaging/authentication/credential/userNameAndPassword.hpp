#ifndef US8_MESSAGING_AUTHENTICATION_CREDENTIAL_USER_NAME_AND_PASSWORD_HPP
#define US8_MESSAGING_AUTHENTICATION_CREDENTIAL_USER_NAME_AND_PASSWORD_HPP
#include <optional>
#include <memory>
namespace US8::Messaging::Authentication::Credential
{
/// @class UserNameAndPassword "userNameAndPassword.hpp" "us8/messaging/authentication/credential/userNameAndPassword.hpp"
/// @brief This credential is defined by a username and, optionally, a 
///        password.
/// @note Unless stated otherwise, it should be assumed that this information
///       will move on an unencrypted network.  This is sensible for operation
///       within the Kubernetes cluster but in the case of Ingress, the user 
///       should verify incoming traffic is encrypted.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
/// @ingroup Authentication_Certificate
class UserNameAndPassword
{
public:
    /// @name Constructors
    /// @{

    /// @brief Constructs the credential from solely a username.  In this case,
    ///        a password could be set later or, if the authenticator does not
    ///        require one, then it need not be set at all.
    explicit UserNameAndPassword(const std::string &userName);
    /// @brief Constructs the credential from the username and password.
    UserNameAndPassword(const std::string &userName, const std::string &password);
    /// @brief Copy constructor.
    /// @param[in] plainText   The certificate from which to initialize
    ///                        this class.
    UserNameAndPassword(const UserNameAndPassword &plainText);
    /// @brief Move constructor.
    /// @param[in,out] plainText  The certificate from which to initialize
    ///                           this class.  On exit, certificate's behavior
    ///                           is undefined.
    UserNameAndPassword(UserNameAndPassword &&plainText) noexcept;
    /// @}

    /// @name Operators
    /// @{

    /// @result A deep copy of the plainText credentials.
    /// @param[in] plainText  The certificate to copy to this.
    /// @result A deep copy of plainText.
    UserNameAndPassword& operator=(const UserNameAndPassword &plainText);
    /// @brief Move assignment operator.
    /// @param[in,out] plainText  The certificate whose memory will be moved to
    ///                           this.  On exit, plainText's behavior is
    ///                           undefined.
    /// @result The memory from plainText moved to this.
    UserNameAndPassword& operator=(UserNameAndPassword &&plainText) noexcept;
    /// @}

    /// @name User Name (Required)
    /// @{

    /// @result The user name.
    /// @throws std::runtime_error if \c haveUserName() is false.
    [[nodiscard]] std::string getUserName() const;
    /// @}

    /// @name Password
    /// @{

    /// @result The password.
    /// @throws std::runtime_error if \c havePassword() is false.
    [[nodiscard]] std::optional<std::string> getPassword() const noexcept;
    /// @}

    /// @name Destructors
    /// @{

    /// @brief Releases all memory and resets the class.
    void clear() noexcept;
    /// @brief Destructor.
    ~UserNameAndPassword(); 
    /// @}

    UserNameAndPassword() = delete;
private:
    class UserNameAndPasswordImpl;
    std::unique_ptr<UserNameAndPasswordImpl> pImpl;
};
}
#endif
