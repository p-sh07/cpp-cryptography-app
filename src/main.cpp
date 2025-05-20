#include "crypto_guard_ctx.h"
#include "cmd_options.h"
#include "util.h"

#include <fstream>
#include <iostream>
#include <print>
#include <stdexcept>
#include <string>


int main(int argc, char *argv[]) {
    using COMMAND_TYPE = CryptoGuard::ProgramOptions::COMMAND_TYPE;
    try {
        CryptoGuard::CryptoGuardCtx cryptoCtx;
        CryptoGuard::ProgramOptions options;

        // 0.Parse cmd-line arguments
        options.Parse(argc, argv);

        // 0.1.If --help or -h was used, program prints help info and exits normally
        if(options.HelpRequested()) {
            std::print("{}\n", options.GetHelpStr());
        }

        // 1.Open required files:
        std::fstream in, out;
        util::OpenFile(in, options.GetInputFile());
        if(options.GetCommand() != COMMAND_TYPE::CHECKSUM) {
            util::OpenFile(out, options.GetOutputFile());
        }

        // 2.Perform command
        switch (options.GetCommand()) {
            case COMMAND_TYPE::ENCRYPT:
                cryptoCtx.EncryptFile(in, out, options.GetPassword());
                break;

            case COMMAND_TYPE::DECRYPT:
                cryptoCtx.DecryptFile(in, out, options.GetPassword());
                break;

            case COMMAND_TYPE::CHECKSUM:
                std::print("Checksum: {}\n", cryptoCtx.CalculateChecksum(in));
                break;

            default:
                throw std::runtime_error{"Unsupported command"};
        }

        // 3.Close all files, 'out' will not throw error if was not opened
        util::CloseFiles(in, out);
    } catch (const std::exception &e) {
        // 4.Process errors
        std::print(std::cerr, "Error: {}\n", e.what());
        return 1;
    }

    return 0;
}
