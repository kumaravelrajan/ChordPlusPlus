FROM gcc:11.2.0 as build

# SETUP
RUN apt-get update
RUN apt-get install -y cmake
WORKDIR /app
COPY CMakeLists.txt .
COPY ./src ./src
COPY ./test ./test
COPY ./cmake ./cmake

# BUILD
RUN mkdir build
WORKDIR build
RUN cmake ..
RUN make
RUN make install

# INSTALL
# TODO

# RUN
WORKDIR /app/workdir
ENTRYPOINT "dht-4"
CMD []

# FROM ubuntu:20.04