#!/bin/bash

cmake -B build
cmake --build build -j
cp configs/* build