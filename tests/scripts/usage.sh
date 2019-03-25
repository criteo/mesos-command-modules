#!/bin/bash

echo '{"timestamp": 12345, "net_snmp_statistics": {
"tcp_stats": { "CurrEstab": 5}
}}' >$2
