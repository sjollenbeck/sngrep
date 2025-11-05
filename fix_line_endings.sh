#!/bin/sh
# Script to fix line endings in bootstrap.sh

echo "Fixing line endings in bootstrap.sh..."

# Method 1: Using sed (most compatible)
if command -v sed >/dev/null 2>&1; then
    echo "Using sed to fix line endings..."
    sed -i 's/\r$//' bootstrap.sh
    echo "Fixed with sed!"
# Method 2: Using tr
elif command -v tr >/dev/null 2>&1; then
    echo "Using tr to fix line endings..."
    tr -d '\r' < bootstrap.sh > bootstrap.sh.tmp
    mv bootstrap.sh.tmp bootstrap.sh
    echo "Fixed with tr!"
# Method 3: Using perl
elif command -v perl >/dev/null 2>&1; then
    echo "Using perl to fix line endings..."
    perl -pi -e 's/\r\n/\n/g' bootstrap.sh
    echo "Fixed with perl!"
else
    echo "ERROR: Could not find sed, tr, or perl to fix line endings"
    echo "Please install one of these tools or use dos2unix"
    exit 1
fi

# Make bootstrap.sh executable
chmod +x bootstrap.sh

echo "Line endings fixed successfully!"
echo "You can now run: ./bootstrap.sh"
