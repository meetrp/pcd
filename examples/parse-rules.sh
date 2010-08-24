#! /bin/sh

@echo This shell script runs pcdparser to generate a graph from the example rules.
../bin/host/pcdparser -f examples/etc/scripts/product.pcd -b examples/ -v -g examples/graph.txt
