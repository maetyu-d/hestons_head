#!/bin/sh
set -eu
cd "$(dirname "$0")"
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j 6
cmake_dir=$(dirname "$(command -v cmake)")
if command -v ctest >/dev/null 2>&1; then
  ctest --test-dir build -C Release --output-on-failure
elif [ -x "$cmake_dir/ctest" ]; then
  "$cmake_dir/ctest" --test-dir build -C Release --output-on-failure
else
  ./build/aftertaste_tests
  ./build/power_tests
  ./build/farm_delivery_tests
  ./build/farm_flow_tests
  ./build/farm_migration_tests
  ./build/factory_tests
  ./build/culinary_tests
  ./build/floor_tests
  ./build/flow_tests
  ./build/glass_tests
fi
if [ "$(uname -s)" = Darwin ]; then
  stage="build/Aftertaste-staged.app"
  cmake -E rm -rf "$stage"
  ditto build/Aftertaste.app "$stage"
  codesign --force --deep --sign - "$stage"
  codesign --verify --deep --strict "$stage"
  if [ -d Aftertaste.app ]; then
    mv Aftertaste.app "build/Aftertaste-previous-$(date +%s).app"
  fi
  mv "$stage" Aftertaste.app
fi
