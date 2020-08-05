# This image is meant to build and test the modules
ARG BUILD_TYPE=automake
FROM criteo/mesosbuild:1.9.0-$BUILD_TYPE

ENV MESOS_BUILD_DIR=/src/mesos/build
ENV MESOS_ROOT_DIR=/src/mesos

COPY scripts/llvm-3.8.0.repo /etc/yum.repos.d/
RUN yum install -y cmake clang-3.8.0 clang-format jq
ENV PATH="${PATH}:/opt/llvm-3.8.0/bin"

VOLUME ["/src/mesos-command-modules"]

WORKDIR /src/mesos-command-modules

CMD ["scripts/travis.sh"]
