#!/bin/bash

echo '{"resources":[{"name":"toto","type":"SCALAR","scalar":{"value":1}}],"message":"too much toto","reason":"REASON_CONTAINER_LIMITATION"}' >$2
