#!/bin/bash
cd `dirname $0`
../../../build/bin/clang-check -ast-dump $1


