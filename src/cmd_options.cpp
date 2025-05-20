#include "cmd_options.h"
#include "util.h"

//#include <print>
#include <iostream>
#include <sstream>

namespace CryptoGuard {
ProgramOptions::ProgramOptions() : desc_("Allowed options") {
    desc_.add_options()
        ("help,h", "print help")
        ("command,c", po::value(&command_string_)->value_name("command"s), "choose command: encrypt, decrypt, checksum")
        ("input,i", po::value(&inputFile_)->value_name("input"s), "input file path")
        ("output,o", po::value(&outputFile_)->value_name("output"s), "output file path")
        ("password,p", po::value(&password_)->value_name("password"s), "encryption password");
}

ProgramOptions::~ProgramOptions() = default;

void ProgramOptions::Parse(int argc, char* argv[]) {

    // variables_map хранит значения опций после разбора
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc_), vm);
    po::notify(vm);

    //Set flag if --h --help options are present.
    if (vm.contains("help"s)) {
        help_requested_ = true;
        return; //no need to process other commands, just print help & exit
    }

    //Check presence of required options:
    if (!vm.contains("command"s)) {
        throw std::invalid_argument("Command is not specified"s);
    }
    command_ = commandMapping_.at(command_string_);

    if (!vm.contains("input"s)) {
        throw std::invalid_argument("Input file path is not specified"s);
    }
    if(!util::FilenameBasicSyntaxIsValid(inputFile_)) {
        throw std::invalid_argument("Input file path is invalid"s);
    }

    //Requires output file and password options for ENCRYPT/DECRYPT
    if(command_ != COMMAND_TYPE::CHECKSUM) {
        if (!vm.contains("output"s)) {
            throw std::invalid_argument("Output file path is not specified"s);
        }
        if(!util::FilenameBasicSyntaxIsValid(outputFile_)) {
            throw std::invalid_argument("Output file path is invalid"s);
        }
        if (!vm.contains("password"s)) {
            throw std::invalid_argument("Encryption password is required!"s);
        }
    }
    // Program options are ok, parsing success. Opts are stored in class vars
}

std::string ProgramOptions::GetHelpStr() const {
    std::stringstream ss;
    ss << desc_;
    return ss.str();
}

} // namespace CryptoGuard
