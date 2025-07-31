#!/bin/bash
set -ex

# ===== CI environment =====
export WINEARCH=win64
export WINEPATH='Z:\usr\lib\gcc\x86_64-w64-mingw32\13-posix'
export WINEPREFIX=$HOME/.wine64
export TZ=UTC

# Use 2 cores like CI
CORES=2
echo "Using $CORES cores"

# ===== Install Mingw packages =====
sudo dpkg --add-architecture i386
sudo apt update
sudo apt install autoconf automake libtool make pkg-config mingw-w64 mingw-w64-tools \
    libz-mingw-w64-dev wine32 wine64 wget unzip lcov gcovr

sudo update-alternatives --set x86_64-w64-mingw32-g++ /usr/bin/x86_64-w64-mingw32-g++-posix

# ===== Fresh Wine Prefix =====
rm -rf "$WINEPREFIX"
wineboot --init || true

# ===== Bootstrap =====
./bootstrap

# ===== Configure with coverage flags =====
./configure --host=x86_64-w64-mingw32 --target=x86_64-w64-mingw32 \
  --disable-java --enable-shared --disable-static \
  CFLAGS="--coverage" LDFLAGS="--coverage"

# ===== Show config =====
cat tsk/tsk_config.h

# ===== Unpack and List test data =====
cd ..
pwd
wget -q https://digitalcorpora.s3.amazonaws.com/corpora/drives/tsk-2024/sleuthkit_test_data.zip
unzip -o sleuthkit_test_data.zip
cd sleuthkit_test_data
make unpack
find . -ls | grep -v '[.]git'
export SLEUTHKIT_TEST_DATA_DIR=$(pwd)
cd ../sleuthkit

# ===== Build =====
make -j$CORES

# ===== Run Tests =====
make -j$CORES check VERBOSE=1 LOG_COMPILER=scripts/wine_wrapper.sh || result=1
for i in $(find test -name '*.log'); do
    printf '\n%79s\n' | tr ' ' '='
    echo "$i"
    cat "$i"
done

# ===== Generate coverage report =====
lcov --capture --gcov-tool x86_64-w64-mingw32-gcov --directory . --output-file coverage.info
lcov --remove coverage.info '/usr/*' --output-file coverage.info
lcov --list coverage.info

# ===== Optional: Upload to Codecov (if token available) =====
if [ -n "$CODECOV_TOKEN" ]; then
    bash <(curl -s https://codecov.io/bash) -f coverage.info -F windows-mingw
fi

exit ${result:-0}
