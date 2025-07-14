#ifndef US8_MESSAGING_AUTHENTICATION_CREDENTIAL_KEY_PAIR_HPP
#define US8_MESSAGING_AUTHENTICATION_CREDENTIAL_KEY_PAIR_HPP
#include <memory>
#include <vector>
namespace US8::Messaging::Authentication::Credential
{
/// @class KeyPair "keyPair.hpp" "us8/messaging/authentication/credential/keyPair.hpp"
/// @brief A class for managing a public/private key-based certificates
///        authenticating a client or server with ZeroMQ and libsodium.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
/// @ingroup Authentication_Certificate
class KeyPair
{
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    KeyPair();
    /// @brief Copy constructor.
    /// @param[in] keys  The certificate from which to initialize this class.
    KeyPair(const KeyPair &keys);
    /// @brief Move constructor.
    /// @param[in,out] keys  The certificate from which to initialize
    ///                      this class.  On exit, keys's behavior
    ///                      is undefined.
    KeyPair(KeyPair &&keys) noexcept;
    /// @}

    /// @name Operators
    /// @{

    /// @brief Copy assignment operator.
    /// @param[in] keys  The certificate to copy to this.
    /// @result A deep copy of the input certificate.
    KeyPair& operator=(const KeyPair &keys);
    /// @brief Move assignment operator.
    /// @param[in,out] keys  The certificate whose memory will be moved
    ///                      to this.  On exit, keys's behavior
    ///                      is undefined.
    /// @result The memory moved from certificate to this.
    KeyPair& operator=(KeyPair &&keys) noexcept;
    /// @}

    /// @name Keypair Creation
    /// @{

    /// @brief This will create a new public/private keypair.
    /// @throws std::runtime_error if an algorithmic error occurred.
    void create();
    /// @}

    /// @name KeyPair
    /// @{

    /// @brief Sets the binary public key.
    /// @param[in] publicKey  The binary public key to set.
    /// @throws std::runtime_error in the case of algorithmic failure.
    void setPublicKey(const std::vector<uint8_t> &publicKey);
    /// @brief Sets the human-readable public key.
    /// @param[in] publicKeyText  The human-readable public key to set.
    /// @throws std::runtime_error in the case of algorithmic failure.
    void setPublicKey(const std::vector<char> &publicKeyText);
    /// @result The public key in a binary format.
    /// @throws std::runtime_error if \c havePublicKey() is false.
    [[nodiscard]] std::vector<uint8_t> getPublicKey() const;
    /// @result The public key in human readable format.
    /// @throws std::runtime_error if \c havePublicKey() is false.
    [[nodiscard]] std::vector<char> getPublicTextKey() const;
    /// @result True indicates that the public key was set.
    [[nodiscard]] bool havePublicKey() const noexcept;

    /// @brief Sets the binary private key.
    /// @param[in] privateKey  The binary private key to set.
    /// @throws std::runtime_error in the case of algorithmic failure.
    void setPrivateKey(const std::vector<uint8_t> &privateKey);
    /// @brief Sets the human-readable private key.
    /// @param[in] privateKeyText  The human-readable private key to set.
    /// @throws std::runtime_error in the case of algorithmic failure.
    void setPrivateKey(const std::vector<char> &privateKeyText);
    /// @result The private key in a binary format.
    /// @throws std::runtime_error if \c havePrivateKey() is false.
    [[nodiscard]] std::vector<uint8_t> getPrivateKey() const;
    /// @result The private key in human readable format.
    /// @throws std::runtime_error if \c havePrivateKey() is false.
    [[nodiscard]] std::vector<char> getPrivateTextKey() const;
    /// @result True indicates that the private key was set.
    [[nodiscard]] bool havePrivateKey() const noexcept;

    /// @brief Sets the keypair from the binary public and private key.
    /// @param[in] publicKey   The binary public key.
    /// @param[in] privateKey  The binary private key.
    /// @throws std::runtime_error in the case of algorithm failure.
    void setPair(const std::vector<uint8_t> &publicKey,
                 const std::vector<uint8_t> &privateKey);
    /// @brief Sets the keypair from a human-readable public and private key.
    /// @param[in] publicKeyText   The human readable public key.
    /// @param[in] privateKeyText  The human readable private key.
    /// @throws std::runtime_error in the case of algorithmic failure.
    void setPair(const std::vector<char> &publicKeyText,
                 const std::vector<char> &privateKeyText);
    /// @result Convenience function that indicates the public and private key
    ///         pair are set.
    [[nodiscard]] bool haveKeyPair() const noexcept;
    /// @}

    /// @name Metadata
    /// @{

    /// @brief Sets the metadata for this key pair.  This can be a description
    ///        like "Client test certificate"
    /// @param[in] metadata   The metadata for this keypair.
    void setMetadata(const std::string &metadata) noexcept;
    /// @result The metadata associated with this key pair.
    [[nodiscard]] std::string getMetadata() const noexcept;
    /// @}

    /// @name File Input/Output
    /// @{

    /// @brief Writes the public key to a text file.
    /// @param[in] fileName  The public key file name.  The recommended suffix
    ///                      is ".public_key".
    /// @throws std::runtime_error if \c havePublicKey() is false.
    void writePublicKeyToTextFile(const std::string &fileName) const;
    /// @brief Writes the private key to a text file.
    /// @param[in] fileName  The public key file name.  The recommended suffix
    ///                      is ".private_key".
    /// @throws std::runtime_error if \c havePrivateKey() is false.
    void writePrivateKeyToTextFile(const std::string &fileName) const;
    /// @brief Reads the public and/or private key from a text file.
    /// @param[in] fileName   The name of the file with the key(s).
    /// @throws std::invalid_argument if the file does not exist.
    /// @throws std::runtime_error if there is a formatting error.
    void loadFromTextFile(const std::string &fileName);
    /// @}


    /// @name Destructors
    /// @{

    /// @brief Resets the class and releases all memory.
    void clear() noexcept;
    /// @brief Destructor.
    ~KeyPair();
    /// @}
private:
    class KeyPairImpl;
    std::unique_ptr<KeyPairImpl> pImpl; 
};
}
#endif
