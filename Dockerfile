# ----------- BUILD ---------------
FROM alpine:3.23.4 AS build

RUN apk update && \
    apk add --no-cache \
        build-base=0.5-r3 \
        cmake=4.1.3-r0

WORKDIR /andromeda

COPY lib/ ./lib/
COPY src/ ./src/
COPY CMakeLists.txt .
COPY CMakePresets.json .

RUN cmake --preset gcc-release && \
    cmake --build --preset gcc-release


# ----------- RUN ---------------
FROM alpine:3.23.4

WORKDIR /app

RUN apk update && \
    apk add --no-cache libstdc++=15.2.0-r2

COPY res/ ./res/
COPY static/ ./static/
COPY templates/ ./templates/
COPY --from=build \
    ./andromeda/build/release/andromeda .

RUN echo "{\"listen_urls\": [ \"http://0.0.0.0:8080\" ], \"db\": \"andromeda.db\"}" > andromeda.json

RUN addgroup -S andromeda && adduser -S andromeda -G andromeda && chown andromeda:andromeda /app
USER andromeda

EXPOSE 8080

ENTRYPOINT [ "./andromeda" ]