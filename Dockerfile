# This image is meant to build and test the modules
FROM kamaradclimber/mesos-build:1.7.0-centos7

ENV MESOS_BUILD_DIR=/src/mesos/build

ADD scripts/llvm-3.8.0.repo /etc/yum.repos.d/
RUN yum install -y cmake clang-3.8.0
ENV PATH="${PATH}:/opt/llvm-3.8.0/bin"
