#!/bin/bash

set -ex

clang-format -i $(find tests src -type f -name '*.h' -or -name '*.cpp')

