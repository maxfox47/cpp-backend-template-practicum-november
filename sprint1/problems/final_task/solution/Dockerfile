# Создать образ на основе базового слоя gcc (там будет ОС и сам компилятор).
# 11.3 — используемая версия gcc.
FROM gcc:11.3 as build

# Выполнить установку зависимостей внутри контейнера.
RUN apt update && \
    apt install -y \
      python3-pip \
      cmake \
    && \
    pip3 install "conan<2.0"

# Создаем default профиль Conan
RUN conan profile new default --detect --force
COPY conanfile.txt /app/
RUN mkdir /app/build && cd /app/build && \
    conan install .. --build=missing

# Скопировать файлы проекта внутрь контейнера
COPY ./src /app/src
COPY ./data /app/data
COPY CMakeLists.txt /app/

# новая команда для сборки сервера:
RUN cd /app/build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    cmake --build . 

# Второй контейнер в том же докерфайле
FROM ubuntu:22.04 as run

# Создадим пользователя mak
RUN groupadd -r mak && useradd -r -g mak mak
USER mak

# Скопируем приложение со сборочного контейнера в директорию /app.
# Не забываем также папку data, она пригодится.
COPY --from=build /app/build/bin/game_server /app/
COPY ./data /app/data

# Запускаем игровой сервер
ENTRYPOINT ["/app/game_server", "/app/data/config.json"]
