#!/bin/bash

make_path="external/owl"
cp_from="external/owl/lib/libowl.a"
cp_to="external/lib/"

# build owl for the current arch
make -C $make_path clean && make -C $make_path && cp $cp_from $cp_to