#!/bin/bash

set -ex

clang-format -i $(find tests src python tests -type f -name '*.h' -or -name '*.cpp')

isort python
black python
