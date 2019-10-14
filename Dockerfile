# This image is meant to build and test the modules
FROM criteo/mesosbuild:1.8.1

ENV MESOS_BUILD_DIR=/src/mesos/build
ENV MESOS_ROOT_DIR=/src/mesos

ADD scripts/llvm-3.8.0.repo /etc/yum.repos.d/
RUN yum install -y cmake clang-3.8.0
ENV PATH="${PATH}:/opt/llvm-3.8.0/bin"

VOLUME ["/src/mesos-command-modules"]

WORKDIR /src/mesos-command-modules

CMD ["sh", "-c", "mkdir -p build && cd build && cmake3 .. && make -j4 && TEST_RESOURCES_PATH=../tests/scripts/ make check"]
