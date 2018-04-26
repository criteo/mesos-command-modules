## Mesos Command Modules

This repository is a collection of **experimental** mesos modules
delegating the events to external commands.

Here is the list of supported modules:
- Hook (all methods)
- Isolator (prepare and cleanup methods only)

This project is not battle-tested, use it at your own risk.

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
