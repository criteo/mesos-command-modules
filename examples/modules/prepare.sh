#!/bin/bash

INPUT_FILE=$1
INPUT=`cat $INPUT_FILE`

echo "$INPUT" > /tmp/prepared
