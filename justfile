dev:
  rm -rf ./dist
  rm -rf ./build
  zig build
  uv build --wheel
  uv pip install dist/*.whl

build-c:
  make -j 4
