#!/bin/bash

set -ex

cmake -S . -B build
cmake --build build

build/tests/testsuite

# TODO: build a real python package here.
pip install -r dev-requirements.txt
PYTHONPATH=build/python pytest -vs python
