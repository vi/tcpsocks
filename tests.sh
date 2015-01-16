#!/bin/bash

printf "Basic test:    "

./tcpsocks 127.0.0.1 7777 4.5.6.7 12345 127.0.0.1 7778 < /dev/null > /dev/null &
T_P=$!

trap "kill -9 $T_P" EXIT

socat tcp-l:7778,fork,reuseaddr exec:./fake_socks.sh &
S_P=$!

trap "kill -9 $T_P $S_P" EXIT

sleep 1

OUT=`echo Hello, | nc -q 1 127.0.0.1 7777`

if [ "$OUT" != "world." ]; then    echo "FAIL, OUT=$OUT"; exit 1; fi

echo "OK"

