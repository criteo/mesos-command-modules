#ifndef __COMMAND_ISOLATOR_HPP__
#define __COMMAND_ISOLATOR_HPP__

#include <string>

#include "Command.hpp"

#include <mesos/module/isolator.hpp>
#include <mesos/slave/isolator.hpp>

#include <stout/option.hpp>

namespace criteo {
namespace mesos {

// Forward declaration
class CommandIsolatorProcess;

/**
 * Isolator calling external commands to handle isolator events.
 *
 * The external command is executed in a fork so that it does not threaten
 * the Mesos agent. The isolator is also protected from infinite loop by
 * killing the child process after a certain amount of time if it does not
 * exit.
 */
class CommandIsolator : public ::mesos::slave::Isolator {
 public:
  /*
   * Constructor
   * In the case a command is empty, the corresponding
   * isolation primitive will not be treated.
   *
   * @param prepareCommand The command to execute when `prepare` event is
   *   triggered for a given container.
   * @param cleanupCommand The command to execute when `cleanup` event is
   *   triggered for a given container.
   * @param watchCommand The command to execute when `watch` event is
   *   triggered  for a given container. This command will be frequently called
   * @param usageCommand The command to execute when `usage` event is triggered
   *   for a given container. This command will be frequently called
   * @param isDebugMode If true, logs inputs and outputs of the commands,
   *   otherwise logs nothing
   */
  explicit CommandIsolator(const Option<Command>& prepareCommand,
                           const Option<Command>& watchCommand,
                           const Option<Command>& cleanupCommand,
                           const Option<Command>& usageCommand,
                           bool isDebugMode = false);

  /**
   * Destructor
   */
  virtual ~CommandIsolator();

  /**
   * Run an external command on prepare phase of a new container.
   * If any execution needs to be done in the containerized context, it can
   * be returned in the optional CommandInfo of ContainerLaunchInfo.
   *
   * @param containerId The id of the container to be created.
   * @param containerConfig The configuration of the container to be created.
   *
   * @return A future resolving an optional ContainerLaunchInfo if successful.
   */
  virtual process::Future<Option<::mesos::slave::ContainerLaunchInfo>> prepare(
      const ::mesos::ContainerID& containerId,
      const ::mesos::slave::ContainerConfig& containerConfig);

  /**
   * Run an external command on cleanup phase of a given container.
   *
   * @param containerId The container ID of the container to be cleaned up.
   *
   * @return A future resolving nothing if successful.
   */
  virtual process::Future<Nothing> cleanup(
      const ::mesos::ContainerID& containerId);

  // Watch the containerized executor and report if any resource
  // constraint impacts the container, e.g., the kernel killing some
  // processes.
  virtual process::Future<::mesos::slave::ContainerLimitation> watch(
      const ::mesos::ContainerID& containerId);

  /**
   * Get prepare command.
   */
  const Option<Command>& prepareCommand() const;

  /**
   * Get cleanup command.
   */
  const Option<Command>& cleanupCommand() const;

  // Get resource usage of a given container
  virtual process::Future<::mesos::ResourceStatistics> usage(
      const ::mesos::ContainerID& containerId);

 private:
  CommandIsolatorProcess* m_process;
};
}  // namespace mesos
}  // namespace criteo

#endif
