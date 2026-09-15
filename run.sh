#!/usr/bin/env bash
set -e
make
qemu-system-i386 -drive format=raw,file=seng21213.img -serial stdio
