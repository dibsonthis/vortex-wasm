#!/bin/bash

.wasm/emscripten/tools/file_packager.py .wasm/vortex.data --preload src --js-output=.wasm/loader.js --from-emcc