#include "CommandQoSController.hpp"
#include "CommandRunner.hpp"
#include "Helpers.hpp"

#include <google/protobuf/util/json_util.h>

#include <process/defer.hpp>
#include <process/dispatch.hpp>

using ::mesos::modules::Module;
using ::mesos::ResourceUsage;
using ::mesos::slave::QoSCorrection;

using std::list;
using std::string;

using process::Future;
using process::Failure;

namespace criteo {

namespace mesos {

class CommandQoSControllerProcess
    : public process::Process<CommandQoSControllerProcess> {
 public:
  CommandQoSControllerProcess(
      const lambda::function<Future<ResourceUsage>()>& usage,
      const Option<Command>& correctionsCommand, bool isDebugMode);

  virtual process::Future<list<::mesos::slave::QoSCorrection>> corrections();
  virtual list<::mesos::slave::QoSCorrection> m_corrections(
      const ResourceUsage& m_usage);
  virtual inline const Option<Command>& correctionsCommand() const {
    return m_correctionsCommand;
  }

 protected:
  const lambda::function<Future<ResourceUsage>()> m_usage;

 private:
  const Option<Command> m_correctionsCommand;
  bool m_isDebugMode;
};

CommandQoSControllerProcess::CommandQoSControllerProcess(
    const lambda::function<process::Future<ResourceUsage>()>& usage,
    const Option<Command>& correctionsCommand, bool isDebugMode)
    : m_usage(usage),
      m_correctionsCommand(correctionsCommand),
      m_isDebugMode(isDebugMode) {}

list<::mesos::slave::QoSCorrection> CommandQoSControllerProcess::m_corrections(
    const ResourceUsage& m_usage) {
  LOG(DEBUG) << "Start Corrections timer_start_module QoSController";
  list<QoSCorrection> listCorrection;
  if (m_correctionsCommand.isNone()) {
    return list<QoSCorrection>();
    ;
  }
  logging::Metadata metadata = {"0", "corrections"};

  CommandRunner cmdRunner = CommandRunner(m_isDebugMode, metadata);

  string input;
  google::protobuf::util::MessageToJsonString(m_usage, &input);
  LOG(DEBUG) << "Start QoSscript timer_start_script QoSController";
  Try<string> output = cmdRunner.run(m_correctionsCommand.get(), input);
  LOG(DEBUG) << "End QoSscript timer_end_script QoSController";

  if (output.isError()) {
    LOG(INFO) << output.error();
    return list<QoSCorrection>();
  }
  // turn the json into QoSCorrections
  // need to turn a string into list of message, going to use split
  // actual output = {QoSCorrection}; {QoSCorrection};...
  // wished output = [{QoSCorrection}, {QoSCorrection}, ...]

  std::string stroutput = output.get();
  std::string delimiter = ";";
  std::string strCorrection;
  std::string test;
  size_t pos = 0;
  ::mesos::slave::QoSCorrection correction;
  correction.set_type(::mesos::slave::QoSCorrection_Type_KILL);

  while ((pos = stroutput.find(delimiter)) != std::string::npos) {
    strCorrection = stroutput.substr(0, pos);
    google::protobuf::util::JsonStringToMessage(strCorrection, &correction);
    google::protobuf::util::MessageToJsonString(correction, &test);
    listCorrection.push_back(correction);
    stroutput.erase(0, pos + delimiter.length());
  }
  LOG(INFO) << "End corrections timer_end_module QoSController";
  return listCorrection;
};

Future<list<QoSCorrection>> CommandQoSControllerProcess::corrections() {
  return m_usage().then(defer(self(), &Self::m_corrections, lambda::_1));
};

CommandQoSController::CommandQoSController(const std::string& name,
                                           const Option<Command>& _corrections,
                                           bool isDebugMode)
    : m_correctionsCommand(_corrections), m_isDebugMode(isDebugMode) {
};

CommandQoSController::~CommandQoSController() {
  if (process != nullptr) {
    terminate(process);
    wait(process);
  }
}

Try<Nothing> CommandQoSController::initialize(
    const lambda::function<Future<::mesos::ResourceUsage>()>& usage) {
  process = new CommandQoSControllerProcess(usage, m_correctionsCommand,
                                            m_isDebugMode);
  spawn(process);

  return Nothing();
}

Future<list<QoSCorrection>> CommandQoSController::corrections() {
  if (process == nullptr) {
    return Failure("QoS Controller is not initialized");
  }
  return dispatch(process, &CommandQoSControllerProcess::corrections);
};

}  // namespace mesos
}  // namespace criteo
