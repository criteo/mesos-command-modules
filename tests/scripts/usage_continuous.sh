#!/bin/bash

USER=$(cat $1 | jq ".[\"container_config\"][\"user\"]"|sed -e "s/\"//g")
if [ -n "$USER" ]
then
    echo '{"timestamp": 1, "net_snmp_statistics": {"tcp_stats": { "CurrEstab": 5}}}' >$2
else
    echo '{"timestamp": 0, "net_snmp_statistics": {"tcp_stats": { "CurrEstab": 5}}}' >$2
fi
