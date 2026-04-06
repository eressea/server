FROM ubuntu:24.04 AS build

RUN apt update
RUN apt install software-properties-common -y

RUN apt install -y build-essential gcc cmake \
    liblua5.2-dev libtolua-dev libncurses5-dev libsqlite3-dev \
    libcjson-dev libiniparser-dev libexpat1-dev libutf8proc-dev

WORKDIR /workspace

COPY CMakeLists.txt eressea/
COPY cmake/ eressea/cmake/
COPY src/ eressea/src/
COPY include/ eressea/include/
COPY storage/ eressea/storage/
COPY clibs/ eressea/clibs/
COPY crpat/ eressea/crpat/
COPY tools/ eressea/tools/
COPY process/CMakeLists.txt eressea/process/

ENV CMAKE_MODULE_PATH=/workspace/eressea/cmake/Modules

RUN cd eressea && ls --recursive /workspace 
RUN cd eressea && mkdir -p build
RUN cd eressea/build && cmake ..
RUN cd eressea/build && make

FROM ubuntu:24.04

ARG WWWGROUP
ARG WWWUSER
ARG LOCALE

ENV DEBIAN_FRONTEND=noninteractive

RUN apt update && apt install -y locales libcjson1 \
    liblua5.2-0 libsqlite3-0 \
    libiniparser1 libexpat1 libutf8proc3
RUN echo "$LOCALE.UTF-8 UTF-8" >> /etc/locale.gen && locale-gen

ENV LC_ALL=$LOCALE.UTF-8
ENV LANG=$LOCALE.UTF-8
ENV LANGUAGE=$LOCALE:en

RUN userdel -r ubuntu
RUN groupadd --force -g $WWWGROUP eressea
RUN useradd -ms /bin/bash --no-user-group -g $WWWGROUP -u $WWWUSER eressea

COPY --from=build /workspace/eressea/build/eressea /usr/local/bin/
COPY scripts/ /usr/local/share/eressea/scripts/
COPY docker/start-container /usr/local/bin/start-container
RUN chmod 755 /usr/local/bin/start-container

USER $WWWUSER:$WWWGROUP

VOLUME ["/data"]

ENTRYPOINT ["/usr/local/bin/start-container"]
