#!/bin/sh

#############################################################
# Quick rebuild script for sngrep with new columns
#############################################################

echo "======================================"
echo " sngrep Quick Rebuild"
echo "======================================"
echo ""

# Check if configure exists
if [ ! -f "configure" ]; then
    echo "Running bootstrap first..."
    ./bootstrap.sh
    if [ $? -ne 0 ]; then
        echo "Bootstrap failed!"
        exit 1
    fi
fi

# Check if Makefile exists
if [ ! -f "Makefile" ]; then
    echo "Running configure..."
    ./configure --with-openssl --with-pcre
    if [ $? -ne 0 ]; then
        echo "Configure failed!"
        exit 1
    fi
fi

# Clean previous build
echo "Cleaning previous build..."
make clean

# Build with multiple cores
echo "Building sngrep..."
CORES=$(nproc 2>/dev/null || echo 2)
make -j$CORES

if [ $? -eq 0 ]; then
    echo ""
    echo "======================================"
    echo " Build completed successfully!"
    echo "======================================"
    echo ""
    echo "To install: sudo make install"
    echo "To test locally: ./sngrep"
    echo ""
    echo "Test the new columns:"
    echo "1. Run: sudo ./sngrep -r tests/aaa.pcap"
    echo "2. Press F10 to open column selector"
    echo "3. Enable 'Disconnect By' and 'Disc Code'"
    echo "4. Check if data appears in the columns"
else
    echo ""
    echo "======================================"
    echo " Build FAILED!"
    echo "======================================"
    echo "Check the errors above and fix them."
    exit 1
fi
