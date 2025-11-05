#!/bin/sh

#############################################################
# Debug script for disconnect columns
#############################################################

echo "======================================"
echo " Debug Disconnect Columns"
echo "======================================"
echo ""

# Create a debug version of the code
echo "Creating debug version with verbose output..."

cat > debug_patch.c << 'EOF'
// Add this temporarily to sip_call.c in call_update_state() function
// Right after the line: reqresp = msg->reqresp;

fprintf(stderr, "DEBUG: call_update_state - state=%d, reqresp=%d, from=%s, to=%s\n", 
        call->state, reqresp, 
        msg->sip_from ? msg->sip_from : "NULL",
        msg->sip_to ? msg->sip_to : "NULL");

// And in the CANCEL case:
fprintf(stderr, "DEBUG: CANCEL detected - setting disconnect_by=%s\n", 
        msg->sip_from ? msg->sip_from : "NULL");

// And in call_get_attribute() for the disconnect cases:
case SIP_ATTR_DISCONNECT_BY:
    fprintf(stderr, "DEBUG: Getting DISCONNECT_BY - value=%s\n",
            call->disconnect_by ? call->disconnect_by : "NULL");
    if (call->disconnect_by)
        sprintf(value, "%s", call->disconnect_by);
    break;
case SIP_ATTR_DISCONNECT_CODE:
    fprintf(stderr, "DEBUG: Getting DISCONNECT_CODE - value=%s\n",
            call->disconnect_code ? call->disconnect_code : "NULL");
    if (call->disconnect_code)
        sprintf(value, "%s", call->disconnect_code);
    break;
EOF

echo ""
echo "To add debug output, add these fprintf lines to src/sip_call.c"
echo "Then recompile and run with stderr visible:"
echo ""
echo "  make clean && make"
echo "  sudo ./sngrep -r tests/aaa.pcap 2>debug.log"
echo ""
echo "Then check debug.log for the output"
echo ""

# Check if fields are in the binary
echo "Checking if new fields are in the compiled binary..."
if [ -f "./sngrep" ]; then
    echo ""
    echo "Searching for 'disconnect' strings in binary:"
    strings ./sngrep | grep -i disconnect | head -10
    echo ""
    
    echo "Checking symbol table:"
    nm ./sngrep 2>/dev/null | grep -i disconnect | head -5
    echo ""
fi

# Create a simple test SIP CANCEL message
echo "Creating test SIP CANCEL message..."
cat > test_cancel.txt << 'EOF'
CANCEL sip:bob@example.com SIP/2.0
Via: SIP/2.0/UDP 192.168.1.100:5060
From: "Alice" <sip:alice@example.com>;tag=1234
To: <sip:bob@example.com>
Call-ID: test-cancel-12345@example.com
CSeq: 1 CANCEL
Content-Length: 0

EOF

echo "Test CANCEL message created: test_cancel.txt"
echo ""
echo "You can test with tcpreplay or sipp to generate traffic"
echo ""

# Alternative: Use gdb to debug
echo "======================================"
echo " GDB Debug Commands"
echo "======================================"
cat << 'EOF'
gdb ./sngrep
(gdb) break call_update_state
(gdb) break call_get_attribute
(gdb) run -r tests/aaa.pcap
(gdb) print call->disconnect_by
(gdb) print call->disconnect_code
(gdb) print msg->sip_from
(gdb) print msg->sip_to
(gdb) continue
EOF
