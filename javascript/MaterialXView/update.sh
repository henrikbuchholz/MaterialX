# Gitbash script to update the JavaScript build of MaterialX with WebGPU support
#
# Prerequisites:
# - Install emsdk (Emscripten SDK) from https://emscripten.org
# - Make sure CMake and Ninja are installed and available in your PATH

# How to use:
# 1. Copy it to MaterialX root
# 2. Set the EMSDK_LOCATION variable to your actual emsdk path
# 3. In GitBash: Run 
#    sh update-javascript-build.sh

cd ../..

# Clean existing JavaScript build
rm -rf javascript/build
rm -rf javascript/bin
rm -rf javascript/lib

# Set up emscripten (adjust path to your emsdk location)
export EMSDK_LOCATION=D:/Fluent/emsdk
$EMSDK_LOCATION/emsdk_env.bat

# Regenerate CMake configuration for JavaScript
cmake -S . -B javascript/build \
  -DMATERIALX_BUILD_JS=ON \
  -DMATERIALX_EMSDK_PATH=$EMSDK_LOCATION \
  -DMATERIALX_CONTRIB=ON \
  -G Ninja
# Build JavaScript bindings
cmake --build javascript/build --target install --config RelWithDebInfo --parallel 32

# Navigate to MaterialXView and rebuild
cd javascript/MaterialXView
npm install
npm run build

# To start:
# http-server . -p 8001
# open url: http://localhost:8001/dist
