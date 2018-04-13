#!/bin/sh

infinite_loop() {
  while true; do sleep 10; done
}

trap infinite_loop INT TERM

infinite_loop
