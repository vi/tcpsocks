#!/bin/bash

IN1=$(dd bs=1 count=3 2> /dev/null | xxd -p)
# only anonymous auth
if [ "$IN1" != "050100" ]; then    echo "IN1=$IN1" >&2; exit 1; fi
printf '\x05\x00'

IN2=$(dd bs=1 count=10 2> /dev/null | xxd -p)
# 4.5.6.7:12345
if [ "$IN2" != "05010001040506073039" ]; then    echo "IN2=$IN2" >&2; exit 1; fi
printf '\x05\x00\x00\x01\x09\x0B\x0B\x0C\xEE\xEE'

read IN3
if [ "$IN3" != "Hello," ]; then    echo "IN3=$IN3" >&2; exit 1; fi
printf 'world.\n'

exit 0
