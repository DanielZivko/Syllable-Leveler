#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
if [[ "$(uname -s)" != Darwin ]]; then
  echo 'A compilacao do Audio Unit exige um Mac.' >&2
  exit 1
fi
if ! xcrun --find clang++ >/dev/null 2>&1; then
  echo 'Instale o Xcode/Command Line Tools compativel com seu macOS.' >&2
  exit 1
fi
if ! command -v cmake >/dev/null 2>&1; then
  echo 'Instale o CMake 3.22 ou posterior compativel com o Big Sur.' >&2
  exit 1
fi
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64' -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
echo 'Gerado: build/SyllableLeveler_artefacts/Release/AU/Syllable Leveler.component'
echo 'O plugin ainda precisa passar pelo auval e pelos testes no Logic.'
