#include "crypto_guard_ctx.h"

#include <array>
#include <iostream>
#include <ranges>
#include <vector>
#include <sstream>
#include <iomanip>
#include <openssl/evp.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/sha.h>

namespace CryptoGuard {
struct AesCipherParams {
    static const size_t KEY_SIZE = 32;                // AES-256 key size
    static const size_t IV_SIZE  = 16;                // AES block size (IV length)
    const EVP_CIPHER* cipher     = EVP_aes_256_cbc(); // Cipher algorithm

    int encrypt;                             // 1 for encryption, 0 for decryption
    std::array<unsigned char, KEY_SIZE> key; // Encryption key
    std::array<unsigned char, IV_SIZE> iv;   // Initialization vector
};

//Openssl -> err massage wrapper, will print openssl err if occurs
static void ThrowIfOpensslErr(int op_result) {
    using namespace std::literals;
    /** from doc: one way to print errors is by ERR_get_error() -> ERR_error_string(errcode): https://gist.github.com/edwardstock/3c992fb71320391d3639696328a61115
     * More convenient to print errors using ERR_print_errors + BIO: https://docs.openssl.org/1.0.2/man3/ERR_print_errors/#name
     * */
    if (op_result == 1 && ERR_peek_error() == 0) {
        //operation was successful, no errors in queue
        return;
    }
    //Create BIO with custom deleter
    auto bio_deleter = [](BIO* bio) {
        BIO_free(bio);
    };
    std::unique_ptr<BIO, decltype(bio_deleter)> bio_ptr(BIO_new(BIO_s_mem()));

    // char* buf;
    // ERR_error_string_n(ERR_get_error(), buf, 256);
    // auto err_msg = std::string(buf, 256);

    ERR_print_errors(bio_ptr.get());
    char* buf;
    size_t len = BIO_get_mem_data(bio_ptr.get(), &buf);
    auto msg = std::string(buf, len); //copy text to string before freeing bio
    std::cerr << msg << std::endl;
    throw std::runtime_error("EVP operation failed: " + msg);
}

class CryptoGuardCtx::Impl {
public:
    Impl();

    ~Impl();

    void Encrypt(std::iostream& inStream, std::iostream& outStream, std::string_view password);

    void Decrypt(std::iostream& inStream, std::iostream& outStream, std::string_view password);

    std::string ComputeChecksum(std::iostream& inStream);

private:
    static constexpr int AES_BUFFER_LEN = 16;
    static constexpr int SHA256_CHECKSUM_LEN = 32;
    static constexpr std::array<unsigned char, 8> SALT = {'4', '2', '1', '9', '3', '8', '7', '6'};

    static AesCipherParams CreateChiperParamsFromPassword(std::string_view password);

    // Set do_encrypt = false for decryption
    void UseOpensslCtx(bool do_encrypt, std::iostream& inStream, std::iostream& outStream, std::string_view password);

    template<typename RandomIt>
    void WriteDataToStream(std::iostream& outStream, RandomIt data_start, RandomIt data_end);

};

//=================== CryptoGuardCtx::PImpl ==================
//== Implementation of the encryption functions
CryptoGuardCtx::Impl::Impl() {
    OpenSSL_add_all_algorithms();
}

CryptoGuardCtx::Impl::~Impl() {
    EVP_cleanup();
}

void CryptoGuardCtx::Impl::Encrypt(std::iostream& inStream, std::iostream& outStream, std::string_view password) {
    UseOpensslCtx(true, inStream, outStream, password);
}

void CryptoGuardCtx::Impl::Decrypt(std::iostream& inStream, std::iostream& outStream, std::string_view password) {
    UseOpensslCtx(false, inStream, outStream, password);
}

std::string CryptoGuardCtx::Impl::ComputeChecksum(std::iostream& inStream) {
    int buffLen = SHA256_DIGEST_LENGTH;
    std::vector<unsigned char> inBuff(buffLen);
    std::vector<unsigned char> hashResult (buffLen);

    //use Evp
    auto ctx_deleter = [](EVP_MD_CTX* ctx) {
        EVP_MD_CTX_free(ctx);
    };
    std::unique_ptr<EVP_MD_CTX, decltype(ctx_deleter)> mdctx(EVP_MD_CTX_new());
    if (!mdctx) {
        throw std::runtime_error("Failed to create sha256-context");
    }

    ThrowIfOpensslErr(EVP_DigestInit_ex(mdctx.get(), EVP_sha256(), nullptr));

    //check stream before starting read
    if(!inStream) {
        throw std::runtime_error("Failed to read inLen chars from input stream: ");
    }

    //read all blocks of buffLength characters
    while(inStream.read(reinterpret_cast<char*>(inBuff.data()), buffLen)) {
        ThrowIfOpensslErr(EVP_DigestUpdate(mdctx.get(), inBuff.data(), buffLen));
    }

    //finish remaining characters
    buffLen = inStream.gcount();
    ThrowIfOpensslErr(EVP_DigestUpdate(mdctx.get(), inBuff.data(), buffLen));

    //failbit gets set by read of N characters when less than N are read
    if(!inStream.eof() && inStream.bad()) { //so check that end was reached & badbit
        throw std::runtime_error("Failed to read remaining chars from input stream");
    }

    unsigned int result_len;
    ThrowIfOpensslErr(EVP_DigestFinal_ex(mdctx.get(), hashResult.data(), &result_len));

    if(result_len != SHA256_CHECKSUM_LEN) {
        throw std::runtime_error("Incorrect Checksum result length");
    }

    //convert to std::string
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hashResult[i]);
    }
    return ss.str();
}

//TODO: this is insecure?, use pbkdf2-hmac-sha256: https://docs.openssl.org/3.3/man3/EVP_BytesToKey/#notes
AesCipherParams CryptoGuardCtx::Impl::CreateChiperParamsFromPassword(std::string_view password) {
    AesCipherParams params;

    int result = EVP_BytesToKey(params.cipher, EVP_sha256(), SALT.data(),
                                reinterpret_cast<const unsigned char*>(password.data()), password.size(), 1,
                                params.key.data(), params.iv.data());

    if (result == 0) {
        throw std::runtime_error{"Failed to create a key from password"};
    }

    return params;
}

//TODO: change to std::ranges? view?
template<typename RandomIt>
void CryptoGuardCtx::Impl::WriteDataToStream(std::iostream& outStream, RandomIt data_start, RandomIt data_end) {
    if (!outStream) {
        throw std::runtime_error("Failed to write data to output stream");
    }
    for ( ; data_start != data_end; ++data_start) {
        outStream << *data_start;
    }
}

void CryptoGuardCtx::Impl::UseOpensslCtx(bool do_encrypt, std::iostream& inStream, std::iostream& outStream, std::string_view password) {
    auto ctx_deleter = [](EVP_CIPHER_CTX* ctx) {
        EVP_CIPHER_CTX_free(ctx);
    };

    std::unique_ptr<EVP_CIPHER_CTX, decltype(ctx_deleter)> ctx(EVP_CIPHER_CTX_new());
    if (!ctx) {
        throw std::runtime_error("Failed to create EVP_CIPHER_CTX");
    }


    auto params    = CreateChiperParamsFromPassword(password);
    params.encrypt = static_cast<int>(do_encrypt);

    ThrowIfOpensslErr(EVP_CipherInit_ex(ctx.get(), params.cipher, nullptr, params.key.data(), params.iv.data(), params.encrypt));

    int outLen = 0, inLen = AES_BUFFER_LEN;
    std::vector<unsigned char> outBuf(inLen + EVP_MAX_BLOCK_LENGTH);
    std::vector<unsigned char> inBuf(inLen);

    //check stream before starting read
    if(!inStream) {
        throw std::runtime_error("Failed to read inLen chars from input stream");
    }

    //Cppreference example with reinterpret_cast: https://en.cppreference.com/w/cpp/io/basic_istream/read
    while (inStream.read(reinterpret_cast<char*>(inBuf.data()), inLen)) {
        ThrowIfOpensslErr(EVP_CipherUpdate(ctx.get(), outBuf.data(), &outLen, inBuf.data(), inLen));
        WriteDataToStream(outStream, outBuf.begin(), outBuf.begin() + outLen);
    }

    //Failbit gets set by read of N characters when less than N are read
    if(!inStream.eof() && inStream.bad()) { //so check that end was reached & badbit
        throw std::runtime_error("Failed to read remaining chars from input stream");
    }

    //Read all blocks of inLen chars, gcount() (< inLen) chars remain in inBuf
    inLen = inStream.gcount();
    outBuf.clear();
    outBuf.resize(inLen + EVP_MAX_BLOCK_LENGTH);

    //Process remaining chars
    ThrowIfOpensslErr(EVP_CipherUpdate(ctx.get(), outBuf.data(), &outLen, inBuf.data(), inLen));
    WriteDataToStream(outStream, outBuf.begin(), outBuf.begin() + outLen);

    ThrowIfOpensslErr(EVP_CipherFinal_ex(ctx.get(), outBuf.data(), &outLen));
    WriteDataToStream(outStream, outBuf.begin(), outBuf.begin() + outLen);
}

//=================== CryptoGuardCtx ===============
//== Cryptography interface class ==
CryptoGuardCtx::CryptoGuardCtx()
    : pImpl_(std::make_unique<Impl>(/*params?*/)) {
}

CryptoGuardCtx::~CryptoGuardCtx() = default;

void CryptoGuardCtx::EncryptFile(std::iostream& inStream, std::iostream& outStream, std::string_view password) {
    pImpl_->Encrypt(inStream, outStream, password);
}

void CryptoGuardCtx::DecryptFile(std::iostream& inStream, std::iostream& outStream, std::string_view password) {
    pImpl_->Decrypt(inStream, outStream, password);
}

std::string CryptoGuardCtx::CalculateChecksum(std::iostream& inStream) {
    return pImpl_->ComputeChecksum(inStream);
}
} // namespace CryptoGuard