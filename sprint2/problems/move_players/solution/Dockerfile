FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    python3-pip \
    git \
    pkg-config \
 && rm -rf /var/lib/apt/lists/*

# Conan v1 for conanfile.txt flow
RUN pip3 install --no-cache-dir "conan<2" && conan --version

WORKDIR /src
COPY . /src

RUN conan profile new default --detect --force \
 && conan profile update settings.compiler.libcxx=libstdc++11 default \
 && conan install . -if build --build=missing -s build_type=Release -s compiler.libcxx=libstdc++11 \
 && cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
 && cmake --build build -j

FROM ubuntu:22.04 AS runtime
WORKDIR /app
COPY --from=builder /src/build/game_server /app/game_server
COPY --from=builder /src/data /app/data
COPY --from=builder /src/static /app/static

EXPOSE 8080
ENTRYPOINT ["/app/game_server", "/app/data/config.json", "/app/static"]

