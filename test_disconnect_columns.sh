#!/bin/sh

#############################################################
# Test script for disconnect columns functionality
#############################################################

echo "======================================"
echo " Testing Disconnect Columns"
echo "======================================"
echo ""

# Check if sngrep binary exists
SNGREP_BIN=""
if [ -x "./sngrep" ]; then
    SNGREP_BIN="./sngrep"
elif [ -x "/usr/local/bin/sngrep" ]; then
    SNGREP_BIN="/usr/local/bin/sngrep"
elif [ -x "/usr/bin/sngrep" ]; then
    SNGREP_BIN="/usr/bin/sngrep"
else
    echo "ERROR: sngrep binary not found!"
    echo "Please build or install sngrep first."
    exit 1
fi

echo "Using sngrep: $SNGREP_BIN"
echo ""

# Check if test pcap exists
TEST_PCAP=""
if [ -f "tests/aaa.pcap" ]; then
    TEST_PCAP="tests/aaa.pcap"
elif [ -f "aaa.pcap" ]; then
    TEST_PCAP="aaa.pcap"
else
    echo "Warning: Test PCAP file not found."
    echo "You can test with live capture or your own PCAP file."
fi

# Create a test configuration
echo "Creating test configuration..."
cat > test_sngrep.conf << EOF
# Test configuration with disconnect columns enabled
set cl.column0 index
set cl.column0.width 4
set cl.column1 sipfrom
set cl.column1.width 20
set cl.column2 sipto
set cl.column2.width 20
set cl.column3 state
set cl.column3.width 12
set cl.column4 disconnectby
set cl.column4.width 20
set cl.column5 disconnectcode
set cl.column5.width 15
set cl.column6 msgcnt
set cl.column6.width 5
EOF

echo "Test configuration created: test_sngrep.conf"
echo ""

if [ ! -z "$TEST_PCAP" ]; then
    echo "To test with the sample PCAP:"
    echo "  sudo $SNGREP_BIN -I test_sngrep.conf -r $TEST_PCAP"
    echo ""
fi

echo "To test with live capture:"
echo "  sudo $SNGREP_BIN -I test_sngrep.conf -i any"
echo ""

echo "To test with your PCAP file:"
echo "  sudo $SNGREP_BIN -I test_sngrep.conf -r your_file.pcap"
echo ""

echo "======================================"
echo " Expected Results:"
echo "======================================"
echo "1. The columns 'Disconnect By' and 'Disc Code' should be visible"
echo "2. For CANCELLED calls: should show who sent CANCEL"
echo "3. For REJECTED calls: should show rejection code (4xx)"
echo "4. For BUSY calls: should show busy code (486/600)"
echo "5. For COMPLETED calls: should show who sent BYE"
echo ""

# Option to run automatically if test pcap exists
if [ ! -z "$TEST_PCAP" ]; then
    echo -n "Run test automatically? (y/N): "
    read REPLY
    if [ "$REPLY" = "y" ] || [ "$REPLY" = "Y" ]; then
        echo "Starting sngrep with test configuration..."
        if [ "$EUID" -ne 0 ]; then
            sudo $SNGREP_BIN -I test_sngrep.conf -r $TEST_PCAP
        else
            $SNGREP_BIN -I test_sngrep.conf -r $TEST_PCAP
        fi
    fi
fi
