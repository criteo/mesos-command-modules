#! /bin/sh

INPUT_FILE=$1
OUTPUT_FILE=$2

#$input=""
#while IFS= read -r line
#do
#  echo "$line"
read -r -d '' OUTPUT << EOM
[{
	"name" : "cpus",
	 "type" : "SCALAR",
	 "scalar" : {"value" : "8"},
	 "role" : "*"}]
EOM

cat $INPUT_FILE > /tmp/test
echo $OUTPUT > $OUTPUT_FILE
