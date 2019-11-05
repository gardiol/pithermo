#!/bin/bash
SUDO=$(which sudo)
$SUDO ./pithermo_daemon --mode=main --logs=../ --config=../config --xchange=../cgi-bin
