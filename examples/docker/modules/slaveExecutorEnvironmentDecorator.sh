#!/bin/bash

INPUT_FILE=$1
OUTPUT_FILE=$2

cat $INPUT_FILE

read -r -d '' OUTPUT << EOM
{
  "variables":[
    {"name": "ENV_1", "value": "test1", "type": "VALUE"},
    {"name": "ENV_2", "value": "test2", "type": "VALUE"}
  ]
}
EOM

echo $OUTPUT > $OUTPUT_FILE
