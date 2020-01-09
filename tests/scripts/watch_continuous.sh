#!/bin/bash
USER=$(cat $1 | jq ".[\"container_config\"][\"user\"]"|sed -e "s/\"//g")
if [ -n "$USER" ]
then
    echo '{"resources":[{"name":"toto","type":"SCALAR","scalar":{"value":1}}],"message":"user found","reason":"REASON_CONTAINER_LIMITATION"}' >$2
fi
