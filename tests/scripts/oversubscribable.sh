#! /bin/sh

INPUT_FILE=$1
OUTPUT_FILE=$2

read -r -d '' OUTPUT << EOM
[{
	"name" : "cpus",
	 "type" : "SCALAR",
	 "scalar" : {"value" : "8"},
	 "role" : "*"}]
EOM

echo $OUTPUT > $OUTPUT_FILE
