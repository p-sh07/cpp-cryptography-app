//
// Created by Pavel on 17.05.2025.
//
#include <gtest/gtest.h>
#include <jemalloc/jemalloc.h>

#include<filesystem>
#include<fstream>
#include<print>
#include<string>
#include<string_view>
#include<ranges>
#include<typeinfo>
#include<vector>
#include<stdexcept>

#include "cmd_options.h"


namespace {
using namespace std::literals;

// Helper functions


//================================================================
// Filenames
// struct OptionsCases {
//     const std::string empty = ""s;
//     const std::string all = "--command encrypt --input input.txt --output output.txt --password password__"s;
//     const std::string all_short = "-c encrypt -i input.txt -o output.txt -p password__"s;
//     const std::string help = "--help"s;
//     const std::string help_short = "-h"s;
// };

// std::pair<int, char*> GenArgcArgvFromStr(const std::string& cmd_str) {
//     int argc = std::count(cmd_str.begin(), cmd_str.end(), ' ');
//     char* argv = new char[argc];
//     constexpr auto space = " "sv;
//     int i = 0;
//     for(auto entry : std::views::split(cmd_str, space)
//         | std::views::transform()) {
//         std::print("type is: {}\n", typeid(entry).name());
//         std::print("{} -> {}\n", i, entry);
//         // argv[i] =
//         ++i;
//     }
//     return std::make_pair(argc, argv);
// }

// Test fixture to construct cmd opt class
class TestCmd : public testing::Test {
protected:
    CryptoGuard::ProgramOptions po_;
    // void SetUp() override {
    //
    // }

    // All strings are deleted at proram exit.
    struct CmdOpts {
        //char _const_* allows to use with literals
        static constexpr char const*  empty[] {
            "progname"
        };
        static constexpr char const* all[]{
            "progname", "--command", "encrypt", "--input", "input.txt", "--output", "output.txt", "--password", "password"
        };
        static constexpr char const* all_swapped_order[]{
            "progname",  "--input", "input.txt", "--password", "password", "--command", "encrypt", "--output", "output.txt"
        };
        static constexpr char const* all_short[] {
            "progname", "-c", "decrypt", "-i", "input.txt", "-o", "output.txt", "-p", "password"
        };
        static constexpr char const* out_file_opt_missing[] {
            "progname", "-c", "decrypt", "-i", "input.txt", "output.txt", "-p", "password"
        };
        static constexpr char const* help[] {
            "progname", "--help"
        };
        static constexpr char const* help_short[] {
            "progname", "-h"
        };
        static constexpr char const* checksum[] {
            "progname", "-c", "checksum", "-i", "input.txt"
        };
        static constexpr char const* checksum_in_file_missing[] {
            "progname", "-c", "checksum"
        };
        static constexpr char const* all_invalid_file_name[]{
            "progname",  "--input", "input.txt", "--password", "password", "--command", "encrypt", "--output", "ou\\tp\"ut.txt"
        };
        static constexpr char const* all_empty_file_name[]{
            "progname", "--command", "encrypt", "--input", "", "--output", "output.txt", "--password", "password"
        };
    };
};

using CMD_TYPE = CryptoGuard::ProgramOptions::COMMAND_TYPE;

TEST_F(TestCmd, all) {
    EXPECT_NO_THROW(po_.Parse(9, const_cast<char**>(CmdOpts::all)));
    EXPECT_EQ(CMD_TYPE::ENCRYPT, po_.GetCommand());
    EXPECT_EQ("input.txt"s, po_.GetInputFile());
    EXPECT_EQ("output.txt"s, po_.GetOutputFile());
    EXPECT_EQ("password"s, po_.GetPassword());
}

TEST_F(TestCmd, all_short) {
    EXPECT_NO_THROW(po_.Parse(9, const_cast<char**>(CmdOpts::all_short)));
    EXPECT_EQ(CMD_TYPE::DECRYPT, po_.GetCommand());
    EXPECT_EQ("input.txt"s, po_.GetInputFile());
    EXPECT_EQ("output.txt"s, po_.GetOutputFile());
    EXPECT_EQ("password"s, po_.GetPassword());
}

TEST_F(TestCmd, swap_order) {
    EXPECT_NO_THROW(po_.Parse(9, const_cast<char**>(CmdOpts::all_swapped_order)));
    EXPECT_EQ(CMD_TYPE::ENCRYPT, po_.GetCommand());
    EXPECT_EQ("input.txt"s, po_.GetInputFile());
    EXPECT_EQ("output.txt"s, po_.GetOutputFile());
    EXPECT_EQ("password"s, po_.GetPassword());
}

//Help woth no other options should not result in error
TEST_F(TestCmd, help) {
    EXPECT_NO_THROW(po_.Parse(2, const_cast<char**>(CmdOpts::help)));
    EXPECT_NO_THROW(po_.Parse(2, const_cast<char**>(CmdOpts::help_short)));

    //TODO: does this actually help find mem leak? shows stats
    //malloc_stats_print(NULL, NULL, NULL);
}

TEST_F(TestCmd, checksum) {
    EXPECT_NO_THROW(po_.Parse(5, const_cast<char**>(CmdOpts::checksum)));
}

TEST_F(TestCmd, errors) {
    EXPECT_THROW(po_.Parse(1, const_cast<char**>(CmdOpts::empty)), std::invalid_argument);
    EXPECT_THROW(po_.Parse(8, const_cast<char**>(CmdOpts::out_file_opt_missing)), std::invalid_argument);
    EXPECT_THROW(po_.Parse(3, const_cast<char**>(CmdOpts::checksum_in_file_missing)), std::invalid_argument);

    EXPECT_THROW(po_.Parse(9, const_cast<char**>(CmdOpts::all_invalid_file_name)), std::invalid_argument);
    EXPECT_THROW(po_.Parse(9, const_cast<char**>(CmdOpts::all_empty_file_name)), std::invalid_argument);
}

} //namespace
//ProgramOptions (TEST(ProgramOptions, TestName))