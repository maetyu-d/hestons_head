#!/bin/sh
set -eu
cd "$(dirname "$0")"
if [ ! -d Aftertaste.app ]; then
  ./build.sh
fi
open Aftertaste.app
