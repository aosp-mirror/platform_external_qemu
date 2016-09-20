#!/bin/sh
progstr=$1
progpid=`pgrep -o $progstr`
while [ "$progpid" = "" ]; do
  progpid=`pgrep -o $progstr`
done
gdb -ex continue -p $progpid
