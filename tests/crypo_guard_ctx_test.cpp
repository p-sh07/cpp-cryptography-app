#include <gtest/gtest.h>

#include<filesystem>
#include<fstream>
#include<print>

#include "crypto_guard_ctx.h"
//#define TEST_WITH_FSTREAM

namespace {
using namespace std::literals;

class TestCtx : public testing::TestWithParam<std::string> {
protected:
    //void SetUp() override; //Empty, just load test into src sstream
    //void TearDown() override;
    void LoadIntoSrc(std::string text) {
        src_ << text;
    }

    std::string GetSrcStr() const {
        return src_.str();
    }

    std::string GetEncryptedStr() const {
        return encrypted_.str();
    }

    std::string GetDecryptedStr() const {
        return decrypted_.str();
    }
    CryptoGuard::CryptoGuardCtx cg_;
    std::string_view password_ = "somepassword"sv;
    std::stringstream src_;
    std::stringstream encrypted_;
    std::stringstream decrypted_;
};

// struct InputData {
//     static constexpr std::string short_txt = "This is a cRyPtoGraphy Example"s;
//     static constexpr std::string long_txt  = "Miss Elizabeth Bennet hastened towards the parlour,"
//                                              "where Mr. Darcy was conversing with Lady Catherine.\n"
//                                              "\"Indeed,\" she exclaimed, \"your lordship\'s visit is most unexpected!\"\n"
//                                              "\"I trust,\" replied Darcy, \"that my presence does not disturb your tranquility.\"";
//     static constexpr std::string cyrilic         = "Ёлки-палки, это самый классный тест для криптографического приложения!"s;
//     static constexpr std::string special_symbols = "\\/:*?\"<>|"s;
//     static constexpr std::string empty           = ""s;
//
//     static constexpr std::string unicode = ""s;
//     static constexpr std::string ansi = ""s;
//     // etc.
// };

TEST_P(TestCtx, EncryptDecryptShort) {
    LoadIntoSrc(GetParam());
    EXPECT_NO_THROW(cg_.EncryptFile(src_, encrypted_, password_));
    EXPECT_NO_THROW(cg_.DecryptFile(encrypted_, decrypted_, password_));

    EXPECT_EQ(GetSrcStr(), GetDecryptedStr());

    std::string src_hash, decr_hash;

    EXPECT_NO_THROW(src_hash = cg_.CalculateChecksum(src_));
    EXPECT_NO_THROW(decr_hash = cg_.CalculateChecksum(decrypted_));
    EXPECT_EQ(src_hash, decr_hash);
}

INSTANTIATE_TEST_SUITE_P(
    EncryptDecryptTests,
    TestCtx,
    testing::Values(
        //short str
        "This is a cRyPtoGraphy Example"s,

        //long str
        "Miss Elizabeth Bennet hastened towards the parlour,"
        "where Mr. Darcy was conversing with Lady Catherine.\n"
        "\"Indeed,\" she exclaimed, \"your lordship\'s visit is most unexpected!\"\n"
        "\"I trust,\" replied Darcy, \"that my presence does not disturb your tranquility.\"",

        //Cyrilic
        "Ёлки-палки, это самый классный тест для криптографического приложения!"s,

        //Special symbols
        "\\/:*?\"<>|"s,

        //Empty
        ""s
    ));

TEST_F(TestCtx, TestPasswords) {

}

TEST_F(TestCtx, Errors) {

}
//Сначала написал тесты с fstream, только потом заметил, что по заданию нужно с sstream о_О
//Можно было переписать и сделать возможность переключения между fstream/sstream, но решил что проще переписать заново с sstream
#ifdef TEST_WITH_FSTREAM
namespace fs = std::filesystem;
// Helper functions
bool compareFiles(const std::string& p1, const std::string& p2) {
    std::ifstream f1(p1, std::ifstream::binary | std::ifstream::ate);
    std::ifstream f2(p2, std::ifstream::binary | std::ifstream::ate);

    if (f1.fail() || f2.fail()) {
        return false; //file problem
    }

    // std::print("tellg1: {} tellg2: {}\n", static_cast<int>(f1.tellg()), static_cast<int>(f2.tellg()));
    if (f1.tellg() != f2.tellg()) {
        return false; //size mismatch
    }

    //seek back to beginning and use std::equal to compare contents
    f1.seekg(0, std::ifstream::beg);
    f2.seekg(0, std::ifstream::beg);
    return std::equal(std::istreambuf_iterator<char>(f1.rdbuf()),
                      std::istreambuf_iterator<char>(),
                      std::istreambuf_iterator<char>(f2.rdbuf()));
}

void clearFile(const std::string& file_path) {
    std::ofstream file{file_path, std::ios_base::trunc};
    file.close();
}

bool fileIsEmpty(const std::string file_path)
{
    std::ifstream file{file_path};
    if(!file) {
        return true;
    }
    //since file just opened, should be at beginning; file.tellg() == 0 &&
    return file.peek() == std::ifstream::traits_type::eof();
}
//================================================================
// Filenames
struct Files {
    static constexpr std::string input_src = "input_src.txt"s;
    static constexpr std::string encrypted_fname = "encrypted.txt"s;
    static constexpr std::string decrypted_fname = "decrypted.txt"s;
};

// To use a test fixture, derive a class from testing::Test.
class TestCtx : public testing::Test {
protected:
    void SetUp() override {
        //open files - create src if doesnt exist
        if(!fs::exists(Files::input_src)) {
            //std::print("working dir: {}\n", fs::current_path().string());
            std::ofstream create{Files::input_src, std::ios_base::trunc};
            create << "Hello Encryption World!\n";
            create.close();
        }

        src_.open(Files::input_src, std::fstream::binary | std::fstream::in);
        if(!src_.is_open()) {
            throw std::runtime_error("Unable to open src!");
        }

        //Open files with fstream::app to prevent contents being erased, but go to beginning in case of read
        encrypted_.open(Files::encrypted_fname, std::fstream::in | std::fstream::out | std::fstream::app | std::fstream::binary);


        if(!encrypted_.is_open()) {
            throw std::runtime_error("Unable to open encrypted!");
        }
        decrypted_.open(Files::decrypted_fname, std::fstream::in | std::fstream::out | std::fstream::app | std::fstream::binary);
        decrypted_.seekg(0, decrypted_.beg); //go to beginning of file

        if(!decrypted_.is_open()) {
            throw std::runtime_error("Unable to open decrypted!");
        }
    }

    void TearDown() override {
         //close files
        src_.close();
        encrypted_.close();
        decrypted_.close();
    }

    std::fstream src_;
    std::fstream encrypted_;
    std::fstream decrypted_;
};

//B.Set password and CG class:
std::string_view password = "hello_world"sv;
CryptoGuard::CryptoGuardCtx crypto_guard_ctx;

//1.Test Encryption works with no errors
TEST(HelperFunc, clearOutputFileEncrypted) {
    EXPECT_NO_THROW(clearFile(Files::encrypted_fname));
    EXPECT_NO_THROW(clearFile(Files::decrypted_fname));
}

TEST_F(TestCtx, NoThrowEncrypt) {
    EXPECT_NO_THROW(crypto_guard_ctx.EncryptFile(src_, encrypted_, password));
}

TEST_F(TestCtx, NoThrowDecrypt) {
    EXPECT_NO_THROW(crypto_guard_ctx.DecryptFile(encrypted_, decrypted_, password));
}

//2.Output files not empty
TEST_F(TestCtx, FilesNotEmpty) {
    EXPECT_TRUE(!fileIsEmpty(Files::encrypted_fname));
    EXPECT_TRUE(!fileIsEmpty(Files::decrypted_fname));
}

//3.Test decripted matches original
TEST_F(TestCtx, DecryptedMatchSrc) {
    //Close all files
    TearDown();

    EXPECT_TRUE(compareFiles(Files::input_src, Files::decrypted_fname));
}
#endif
} //namespace
