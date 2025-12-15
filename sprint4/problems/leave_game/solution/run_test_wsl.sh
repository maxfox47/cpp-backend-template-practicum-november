#!/bin/bash

cd ~/cpp-backend-tests-practicum/scripts/sprint4 || exit 1

export IMAGE_NAME="game_server"
export CONFIG_PATH="/path/to/config.json"
export VOLUME_PATH="/tmp/test_volume"
export SERVER_DOMAIN="127.0.0.1"
export SERVER_PORT="8080"

mkdir -p "$VOLUME_PATH"

pytest tests/test_s04_state_serialization.py::test_kill -v
