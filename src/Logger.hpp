#ifndef __LOGGING_LOGGER_HPP__
#define __LOGGING_LOGGER_HPP__

#include <string>

#define TASK_LOG(severity, metadata) \
    LOG(severity) << "[TASK=" << metadata.taskId << ", METHOD=" << metadata.method << "] "

namespace criteo {
namespace mesos {
namespace logging {
struct Metadata {
    std::string taskId;
    std::string method;
};
}
}
}

#endif
