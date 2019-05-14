#!/bin/bash

echo $$ > /tmp/infinite_loop.pid

while true; do sleep 10; done
