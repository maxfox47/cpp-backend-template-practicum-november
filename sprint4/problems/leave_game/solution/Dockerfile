FROM gcc:11.3 AS build

RUN apt update && \
    apt install -y \
      python3-pip \
      python3-pytest \
      cmake \
    && \
    pip3 install conan==1.*

COPY conanfile.txt /app/
RUN mkdir /app/build && cd /app/build && \
    conan install .. -s compiler.libcxx=libstdc++11 -s build_type=Debug


COPY ./src /app/src
COPY ./tests /app/tests
COPY CMakeLists.txt /app/

RUN cd /app/build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    cmake --build .

FROM ubuntu:22.04 AS run

RUN groupadd -r www && useradd -r -g www www
USER www
COPY --from=build /app/build/bin/game_server /app/
COPY --from=build /app/build/bin/model_tests /app/build/
COPY --from=build /app/build/bin/collision_detection_tests /app/build/
COPY ./data /app/data
COPY ./static /app/static
ENTRYPOINT ["/app/game_server", "--config-file", "/app/data/config.json", "--www-root", "/app/static", "--state-file", "/tmp/volume/state", "--save-state-period", "1000"]
