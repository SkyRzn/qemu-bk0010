#!/bin/bash

mkdir build && \
cd build && \
../configure --target-list=k1801vm1-softmmu --disable-werror && \
make -j`nproc`

