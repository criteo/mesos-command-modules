#!/bin/bash

INPUT_FILE=$1
OUTPUT_FILE=$2

read -r -d '' OUTPUT << EOM
{
  "labels": [
    {"key": "LABEL_1", "value": "test1"},
    {"key": "LABEL_2", "value": "test2"}
  ]
}
EOM

echo $OUTPUT > $OUTPUT_FILE
