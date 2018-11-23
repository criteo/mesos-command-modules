#ifndef __COMMAND_RUNNER_HPP__
#define __COMMAND_RUNNER_HPP__

#include <string>

#include <stout/try.hpp>

#include "Logger.hpp"

namespace criteo {
namespace mesos {

class CommandRunner {
public:
    /**
     * @brief CommandRunner
     * @param debug true to publish debug level information, false otherwise.
     * @param loggingMetadata The metadata like task id prepended to logs.
     */
    CommandRunner(bool debug, const logging::Metadata& loggingMetadata);

    /**
     * Run command receiving two paths to temporary files as input. The first is
     * the file containing the serialized input passed to the command and the
     * second one is the file where the command can log its outputs.
     *
     * The command must exit in less than the timeout given as parameter,
     * otherwise the process receives a SIGTERM and then a SIGKILL if it still has
     * not exited. SIGKILL is sent one second after the end of the timeout if the
     * process is still running after SIGTERM.
     *
     * @param command The command to run.
     * @param serializedInput The serialized input passed to the command through
     *   the temporary file.
     * @param timeout The time in seconds for the command to terminate before
     *   being killed.
     * @param debug Flag enabling the debug mode logging all inputs and outputs.
     *
     * @return The output of the command retrieved from the temporary file.
     */
    Try<std::string> run(
        const std::string& command,
        const std::string& serializedInput,
        unsigned int timeout);

private:
    bool m_debug;
    logging::Metadata m_loggingMetadata;
};

}
}

#endif
