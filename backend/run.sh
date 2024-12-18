#!/bin/bash
SUDO=$(which sudo)
$SUDO ./pithermo_daemon --logs=../ --config=../config
