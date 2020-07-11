#!/bin/bash

# WebRTC のビルドに使っていたのと同じバージョンの clang と libcxx を拾ってくる。
#
# 以下の構成になる。
# <output_dir>/llvm/clang
# <output_dir>/llvm/libcxx
# <output_dir>/llvm/tools
if [ $# -lt 2 ]; then
  echo "$0 <webrtc_dir> <output_dir>"
  exit 1
fi

WEBRTC_DIR=$1
OUTPUT_DIR=$2

set -ex

. $WEBRTC_DIR/VERSIONS

mkdir -p $OUTPUT_DIR/llvm

# aarch64 clang バイナリを拾う
pushd $OUTPUT_DIR/llvm
  wget https://releases.llvm.org/6.0.0/clang+llvm-6.0.0-aarch64-linux-gnu.tar.xz
  tar xvf clang+llvm-6.0.0-aarch64-linux-gnu.tar.xz
  mv clang+llvm-6.0.0-aarch64-linux-gnu clang
  rm clang+llvm-6.0.0-aarch64-linux-gnu.tar.xz
popd

# 特定バージョンの libcxx を利用する
pushd $OUTPUT_DIR/llvm
  if [ ! -e libcxx/.git ]; then
    git clone https://chromium.googlesource.com/external/github.com/llvm/llvm-project/libcxx
  fi
  pushd libcxx
    git fetch
    git reset --hard $WEBRTC_SRC_BUILDTOOLS_THIRD_PARTY_LIBCXX_TRUNK
  popd
popd
