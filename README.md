# Mesos Command Modules

This repository is a collection of **experimental** mesos modules
delegating the events to external commands.

Here is the list of supported modules:
- Hook (all methods)
- Isolator (prepare and cleanup methods only)

Warning: this project is not battle-tested yet, use it at your own risk.

## Getting Started

Follow the [build instructions](#build-instructions) to build the modules for
your architecture and your own version of Mesos.

Create or edit the file containing the modules configuration for your Mesos
cluster and add the following libraries with whatever commands you want
to run for each type of events. All commands are optional so they can be
removed from this configuration.

Note: the `debug` parameter is optional and can be set to true if you want to
enable the logging of all inputs received and all outputs produced by the
commands.


```
{
  "libraries": [
    {
      "file": "/tmp/mesos/modules/libmesos_command_modules.so",
      "modules": [
        {
          "name": "com_criteo_mesos_CommandHook",
          "parameters": [
            {
              "key": "hook_slave_run_task_label_decorator",
              "value": "/opt/mesos/modules/slaveRunTaskLabelDecorator.sh"
            },
            {
              "key": "hook_slave_executor_environment_decorator",
              "value": "/opt/mesos/modules/slaveExecutorEnvironmentDecorator.sh"
            },
            {
              "key": "hook_slave_remove_executor_hook",
              "value": "/opt/mesos/modules/slaveRemoveExecutorHook.sh"
            },
            {
              "key": "debug",
              "value": "false"
            }
          ]
        },
        {
          "name": "com_criteo_mesos_CommandIsolator",
          "parameters": [
            {
              "key": "isolator_prepare",
              "value": "/opt/mesos/modules/prepare.sh"
            },
            {
              "key": "isolator_cleanup",
              "value": "/opt/mesos/modules/cleanup.sh"
            },
            {
              "key": "debug",
              "value": "false"
            }
          ]
        }
      ]
    }
  ]
}
```

Make sure this file will be taken into account by adding the
[--modules](http://mesos.apache.org/documentation/latest/configuration/master-and-agent/)
flag to you mesos agents.

Add the hook to the Mesos agent command with the
`--hooks com_criteo_mesos_CommandHook` flag and/or add the isolator with
the `--isolation com_criteo_mesos_CommandIsolator` flag.

Then, restart the Mesos slaves to complete the installation and have a
look at the logs to confirm that your scripts are called when some of your
configured events are triggered.

## Build Instructions

```shell
    $ export MESOS_BUILD_DIR=[ directory where Mesos was BUILT, e.g. ~/repos/mesos/build ]
    $ mkdir build
    $ cd build
    $ cmake ..
    $ make
    $ make test
```

Please note that you must run **clang-format** before commiting your change,
otherwise the Travis job will fail. To apply clang-format, type:

```shell
make clang-format
```

## Deploy a test environment

Follow [examples/README.md](./examples/README.md).

## Design choices

### Forking a process for each event

We prefered forking in order to ensure the statelessness of the commands.
The downside of this choice is that forking a process might be slow but
we do not expect to have billions of calls on each agent anyway.

### Using temporary files as inputs and outputs buffers

We implemented passing inputs and retrieving outputs from the external
commands with temporary files for simplicity. Indeed, a lot of programming
languages can easily read the content of a file while reading a pipe is not
so trivial (see the tests).

## TODO

* Implement all methods of the isolator interface
* Add tests to check the behavior of the CommandRunner when temporary files are
  manually deleted when a script is running.
* Automate integration tests using docker-compose in examples/ to execute it in
  Travis and ensure non-regression.
