#!/bin/bash

# Скрипт для поиска и запуска теста test_kill

echo "Поиск репозитория cpp-backend-tests-practicum..."

# Ищем репозиторий в домашней директории и поддиректориях
TEST_REPO=$(find ~ -type d -name "cpp-backend-tests-practicum" 2>/dev/null | head -1)

if [ -z "$TEST_REPO" ]; then
    echo "Реопозиторий cpp-backend-tests-practicum не найден!"
    echo ""
    echo "Варианты:"
    echo "1. Клонировать репозиторий:"
    echo "   cd ~"
    echo "   git clone <url-to-cpp-backend-tests-practicum>"
    echo ""
    echo "2. Или найти вручную:"
    echo "   find ~ -type d -name 'cpp-backend-tests-practicum'"
    echo ""
    echo "3. Или указать путь вручную:"
    echo "   export TEST_REPO_PATH=/path/to/cpp-backend-tests-practicum"
    exit 1
fi

echo "Найден репозиторий: $TEST_REPO"
cd "$TEST_REPO/scripts/sprint4"

if [ ! -f "tests/test_s04_state_serialization.py" ]; then
    echo "Файл теста не найден в $TEST_REPO/scripts/sprint4/tests/"
    echo "Проверьте структуру репозитория:"
    ls -la "$TEST_REPO/scripts/sprint4/" 2>/dev/null || echo "Директория не существует"
    exit 1
fi

echo "Установка зависимостей..."
pip3 install pytest docker -q

echo "Проверка Docker..."
if ! command -v docker &> /dev/null; then
    echo "Docker не установлен. Установите Docker для WSL."
    exit 1
fi

# Запуск Docker если не запущен
sudo service docker start 2>/dev/null || sudo systemctl start docker 2>/dev/null || true

echo ""
echo "Установите переменные окружения перед запуском:"
echo "export IMAGE_NAME=\"game_server\""
echo "export CONFIG_PATH=\"/path/to/config.json\""
echo "export VOLUME_PATH=\"/tmp/test_volume\""
echo "export SERVER_DOMAIN=\"127.0.0.1\""
echo "export SERVER_PORT=\"8080\""
echo ""
echo "Затем запустите:"
echo "pytest tests/test_s04_state_serialization.py::test_kill -v -s"

