# Mesos Command Modules

[![Build Status](https://api.travis-ci.org/criteo/mesos-command-modules.svg?branch=master)](https://travis-ci.org/criteo/mesos-command-modules)

This repository is a collection of mesos modules
delegating the events to external commands.

Here is the list of supported modules:
- Hook (all methods)
- Isolator (prepare, cleanup, watch, usage methods)

## Production readyness

This project has been running in production on our infrastructure for since June 2018. It is called on every container start/stop and to monitor metrics.
It represent around 4000 calls / second.

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
              "key": "hook_slave_run_task_label_decorator_command",
              "value": "/opt/mesos/modules/slaveRunTaskLabelDecorator.sh"
            },
            {
              "key": "hook_slave_executor_environment_decorator_command",
              "value": "/opt/mesos/modules/slaveExecutorEnvironmentDecorator.sh"
            },
            {
              "key": "hook_slave_remove_executor_hook_command",
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
              "key": "isolator_prepare_command",
              "value": "/opt/mesos/modules/prepare.sh"
            },
            {
              "key": "isolator_prepare_timeout",
              "value": "10"
            },
            {
              "key": "isolator_cleanup_command",
              "value": "/opt/mesos/modules/cleanup.sh"
            },
            {
              "key": "isolator_watch_command",
              "value": "/opt/mesos/modules/watch.sh"
            },
            {
              "key": "isolator_watch_frequence",
              "value": "10"
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

Note: com_criteo_mesos_CommandIsolator2, com_criteo_mesos_CommandIsolator3, ... are also defined to allow to have several distinct isolators.

## Build Instructions

```shell
    $ export MESOS_ROOT_DIR=[ directory where Mesos was cloned, e.g. ~/repos/mesos ]
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

### With docker

```shell
    $ docker build . -t mesos-command-modules
    $ docker run -v "$(pwd):/src/mesos-command-modules" mesos-command-modules
```

## Test on a slave

```shell
    $ ./scripts/build_and_upload_module.sh user@hostname
```

## Deploy a test environment

Follow [examples/README.md](./examples/README.md).

## Design choices

### Forking a process for each event

We prefered forking in order to ensure the statelessness of the commands.
The downside of this choice is that forking a process might be slow but
we do not expect to have billions of calls on each agent anyway.

Warning: the usage method of Isolator can actually be called very often (on every call for /monitor/statistics endpoint is called which call usage for every container each time). It make a lot of call.

### Using temporary files as inputs and outputs buffers

We implemented passing inputs and retrieving outputs from the external
commands with temporary files for simplicity. Indeed, a lot of programming
languages can easily read the content of a file while reading a pipe is not
so trivial (see the tests).

## TODO

* Add tests to check the behavior of the CommandRunner when temporary files are
  manually deleted when a script is running.
* Automate integration tests using docker-compose in examples/ to execute it in
  Travis and ensure non-regression.
