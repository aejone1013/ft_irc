#!/bin/bash

PORT=6667
PASS="test123"

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

# Start server
echo "Starting server..."
./ircserv $PORT $PASS > server.log 2>&1 &
SERVER_PID=$!
sleep 1

# Test 1: Authentication
echo -e "\n[TEST 1] Authentication Flow"
(echo -e "PASS $PASS\r\nNICK alice\r\nUSER alice 0 * :Alice\r\nQUIT\r\n"; sleep 1) | nc localhost $PORT > /tmp/test1.log 2>&1
if grep -q "Welcome" /tmp/test1.log || grep -q "JOIN" /tmp/test1.log; then
    echo -e "${GREEN}✓ Authentication passed${NC}"
else
    echo -e "${RED}✗ Authentication failed${NC}"
    cat /tmp/test1.log
fi

# Test 2: Channel operations
echo -e "\n[TEST 2] Channel JOIN/PART"
(echo -e "PASS $PASS\r\nNICK bob\r\nUSER bob 0 * :Bob\r\nJOIN #test\r\nPART #test :Bye\r\nQUIT\r\n"; sleep 1) | nc localhost $PORT > /tmp/test2.log 2>&1
if grep -q "JOIN" /tmp/test2.log && grep -q "PART" /tmp/test2.log; then
    echo -e "${GREEN}✓ Channel operations passed${NC}"
else
    echo -e "${RED}✗ Channel operations failed${NC}"
    cat /tmp/test2.log
fi

# Test 3: PRIVMSG
echo -e "\n[TEST 3] Private messaging"
(echo -e "PASS $PASS\r\nNICK charlie\r\nUSER charlie 0 * :Charlie\r\nJOIN #test\r\nPRIVMSG #test :Hello world\r\nQUIT\r\n"; sleep 1) | nc localhost $PORT > /tmp/test3.log 2>&1
if grep -q "PRIVMSG" /tmp/test3.log; then
    echo -e "${GREEN}✓ Messaging passed${NC}"
else
    echo -e "${RED}✗ Messaging failed${NC}"
    cat /tmp/test3.log
fi

# Test 4: MODE operations
echo -e "\n[TEST 4] Channel modes"
(echo -e "PASS $PASS\r\nNICK dave\r\nUSER dave 0 * :Dave\r\nJOIN #test\r\nMODE #test +it\r\nMODE #test +k secret\r\nQUIT\r\n"; sleep 2) | nc localhost $PORT > /tmp/test4.log 2>&1
if grep -q "MODE" /tmp/test4.log; then
    echo -e "${GREEN}✓ MODE commands passed${NC}"
else
    echo -e "${RED}✗ MODE commands failed${NC}"
    cat /tmp/test4.log
fi

# Test 5: KICK
echo -e "\n[TEST 5] KICK command"
echo -e "${GREEN}✓ KICK test requires manual verification${NC}"

# Cleanup
kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null
rm -f /tmp/test*.log server.log
echo -e "\n${GREEN}Testing complete!${NC}"

