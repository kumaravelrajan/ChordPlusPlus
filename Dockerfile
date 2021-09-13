FROM ubuntu:20.04 as build

# SETUP
RUN apt-get update
ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get install -y git
RUN apt-get install -y zlib1g-dev
RUN apt-get install -y clang
RUN apt-get install -y cmake
WORKDIR /app
COPY CMakeLists.txt .
COPY ./src ./src
COPY ./test ./test
COPY ./cmake ./cmake

# BUILD
RUN mkdir build
WORKDIR build
ARG asdasdasdasd=1
RUN cmake ..
RUN make

# INSTALL
RUN make install

# RUN
WORKDIR /app/workdir
ENTRYPOINT ["dht-4"]

# FROM ubuntu:20.04