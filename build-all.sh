#! /bin/sh

# Cross compiler
# export CC=armeb-linux-uclibceabi-gcc

# Include directory prefix
# export INCLUDE_DIR_PREFIX=/projs/rt-embedded/include

# Installation directory prefix
# export INSTALL_DIR_PREFIX=/projs/rt-embedded/filesystem

# IPC library version. Update according to the installed version.
export IPC_VERSION=1.0.1

# PCD version. Update according to the installed version.
export PCD_VERSION=1.0.2

echo "================================"
echo PCD version: $PCD_VERSION
echo IPC version: $IPC_VERSION
echo CC=${CC:-<native>}
echo Include dir=${INCLUDE_DIR_PREFIX:-<none>}
echo Install dir=${INSTALL_DIR_PREFIX:-<none>}
echo "================================"

echo "Building ipc library..."
make -C ipc/src

echo "Building pcdapi library..."
make -C pcd/src/pcdapi/src

echo "Building pcd application..."
make -C pcd/src

echo "Building pcdparser application..."
make -C pcd/src/parser/src

