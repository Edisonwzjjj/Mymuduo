FROM ubuntu:24.04
CMD bash
LABEL authors="wzj"

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get -y update &&  \
    apt-get -y upgrade && \
    apt-get -y install  \
      build-essential \
            clang \
            clang-format\
            clang-tidy \
            make \
            cmake \
            doxygen \
            git \
            g++ \
            gdb  \
            pkg-config \
            zlib1g-dev \
            python3



