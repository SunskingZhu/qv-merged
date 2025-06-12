#!/bin/bash
# This builds a complete qimgv-x64 package. Result is placed in qimgv/qimgv-x64_<version>
# Warning: Some stuff will be left over behind after building (C:/qt and C:/opencv-4.5.5-minimal)

#CFL='-ffunction-sections -fdata-sections -march=native -mtune=native -O3 -pipe'
CFL='-ffunction-sections -fdata-sections -O3 -pipe'
LDFL='-Wl,--gc-sections'

if [[ -z "$RUNNER_TEMP" ]]; then
  MSYS_DIR="C:/msys64/mingw64"
else
  MSYS_DIR="$(cd "$RUNNER_TEMP" && pwd)/msys64/mingw64"
fi
CUSTOM_QT_DIR="C:/qt/5.15.3-mingw64-slim"
OPENCV_DIR="C:/opencv-minimal-4.5.5"
SCRIPTS_DIR=$(dirname $(readlink -f $0)) # this file's location (/path/to/qimgv/scripts)
SRC_DIR=$(dirname $SCRIPTS_DIR)
BUILD_DIR=$SRC_DIR/build
EXT_DIR=$SRC_DIR/_external
MPV_DIR=$EXT_DIR/mpv

# ------------------------------------------------------------------------------
echo "BUILDING"
sed -i 's|opencv4/||' $SRC_DIR/qimgv/3rdparty/QtOpenCV/cvmatandqimage.{h,cpp}
cmake -S $SRC_DIR -B $BUILD_DIR -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH=$CUSTOM_QT_DIR \
    -DOpenCV_DIR=$OPENCV_DIR \
    -DOPENCV_SUPPORT=ON \
    -DVIDEO_SUPPORT=ON \
    -DMPV_DIR=$MPV_DIR \
    -DCMAKE_CXX_FLAGS="$CFL" -DCMAKE_EXE_LINKER_FLAGS="$LDFL"
ninja -C $BUILD_DIR

# ------------------------------------------------------------------------------
echo "PACKAGING"
# 0 - prepare dir
cd $SRC_DIR
BUILD_NAME=qimgv-x64_$(git describe --tags)
PACKAGE_DIR=$SRC_DIR/$BUILD_NAME

# 1 - copy qimgv build
cp $BUILD_DIR/qimgv/qimgv.exe $PACKAGE_DIR

cd $SRC_DIR
echo "PACKAGING DONE"
