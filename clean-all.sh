#! /bin/sh

echo "Cleaning ipc library..."
make -C ipc/src clean

echo "Cleaning pcdapi library..."
make -C pcd/src/pcdapi/src clean

echo "Cleaning pcd application..."
make -C pcd/src clean

echo "Cleaning pcdparser application..."
make -C pcd/src/parser/src clean

