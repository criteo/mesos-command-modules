#!/bin/bash

INPUT_FILE=$1
OUTPUT_FILE=$2

read -r -d '' OUTPUT << EOM
{
  "user": "app_user",
  "rootfs": 4
}
EOM

echo $OUTPUT > $OUTPUT_FILE
