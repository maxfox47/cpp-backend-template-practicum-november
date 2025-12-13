#!/bin/bash

# Команды для запуска теста test_kill на WSL

# 1. Перейти в директорию с тестами (предполагается, что репозиторий находится рядом)
cd ~/cpp-backend-tests-practicum/scripts/sprint4

# 2. Установить зависимости Python (если еще не установлены)
# pip3 install pytest docker

# 3. Убедиться, что Docker запущен
# sudo service docker start

# 4. Установить переменные окружения (замените на ваши значения)
export IMAGE_NAME="game_server"  # Имя вашего Docker образа
export CONFIG_PATH="/path/to/config.json"  # Путь к config.json
export VOLUME_PATH="/tmp/test_volume"  # Путь для volume
export SERVER_DOMAIN="127.0.0.1"
export SERVER_PORT="8080"

# 5. Создать директорию для volume если не существует
mkdir -p "$VOLUME_PATH"

# 6. Запустить конкретный тест test_kill
pytest tests/test_s04_state_serialization.py::test_kill -v

# Или запустить все тесты из файла:
# pytest tests/test_s04_state_serialization.py -v

# Или запустить только test_kill с выводом:
# pytest tests/test_s04_state_serialization.py::test_kill -v -s

