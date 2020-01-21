#! /bin/sh

usage() {
  cat <<EOF
usage: ${0##*/} user@hostname
EOF
}

if [ "$#" -ne "1" ]; then
  usage
  exit 1
fi

set -e -x

SERVER="$1"

docker build . -t mesos-command-modules
docker run -v "$(pwd):/src/mesos-command-modules" mesos-command-modules
scp "$(pwd)/build/libmesos_command_modules.so" "$SERVER":
ssh "$SERVER" sudo cp libmesos_command_modules.so /usr/lib64/mesos/modules/libmesos_command_modules.so
ssh "$SERVER" sudo systemctl restart mesos-slave
