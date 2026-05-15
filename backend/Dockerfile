FROM gcc:14 AS build

RUN apt-get update \
    && apt-get install -y --no-install-recommends cmake ninja-build \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY CMakeLists.txt ./
COPY application ./application
COPY domain ./domain
COPY include ./include
COPY infrastructure ./infrastructure
COPY src ./src
COPY tests ./tests
COPY migrations ./migrations

RUN cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF \
    && cmake --build build --target nexustal_engine

FROM debian:bookworm-slim AS runtime

RUN useradd --system --create-home --shell /usr/sbin/nologin nexustal

WORKDIR /opt/nexustal
COPY --from=build /src/build/nexustal_engine /opt/nexustal/nexustal_engine
COPY migrations /opt/nexustal/migrations

RUN chown -R nexustal:nexustal /opt/nexustal
USER nexustal

EXPOSE 8080 9001
CMD ["/opt/nexustal/nexustal_engine"]
