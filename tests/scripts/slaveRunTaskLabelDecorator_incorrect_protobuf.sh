#!/bin/bash

INPUT_FILE=$1
OUTPUT_FILE=$2

cat $INPUT_FILE

read -r -d '' OUTPUT << EOM
{
  "labels": [
    {"bad_key": "LABEL_1", "value": "test1"},
    {"bad_key": "LABEL_2", "value": "test2"}
  ]
}
EOM

echo $OUTPUT > $OUTPUT_FILE
