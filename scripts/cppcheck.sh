#!/bin/sh

cd "${MESON_SOURCE_ROOT}"

cppcheck src/ --enable=all --inconclusive --quiet --verbose \
	--std=c99 --std=posix \
	--force -isrc/libs/ \
	--output-file="${MESON_BUILD_ROOT}/cppcheck_result.txt"
