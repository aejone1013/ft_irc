# ft_irc - IRC Server

42 School project implementing an IRC server in C++98.

## Features

✅ Multi-client support with poll() multiplexing  
✅ Non-blocking I/O  
✅ Authentication: PASS, NICK, USER  
✅ Channels: JOIN, PART, TOPIC, INVITE  
✅ Messaging: PRIVMSG  
✅ Operators: KICK, MODE (i,t,k,o,l)  
✅ Graceful disconnect: QUIT  

## Quick Start

```bash
# Compile
make

# Run server
./ircserv 6667 mypassword

# Connect with netcat
nc localhost 6667
PASS mypassword
NICK mynick
USER myuser 0 * :My Real Name
JOIN #channel
PRIVMSG #channel :Hello!

# Or use HexChat/irssi
```

## Commands

| Command | Format | Description |
|---------|--------|-------------|
| PASS | `PASS <password>` | Authenticate |
| NICK | `NICK <nickname>` | Set nickname |
| USER | `USER <user> 0 * :<realname>` | Register |
| JOIN | `JOIN <#channel> [<key>]` | Join channel |
| PART | `PART <#channel> [:<msg>]` | Leave channel |
| PRIVMSG | `PRIVMSG <target> :<message>` | Send message |
| KICK | `KICK <#channel> <user> [:<reason>]` | Kick user (op) |
| MODE | `MODE <#channel> <+/-modes> [<params>]` | Set modes (op) |
| TOPIC | `TOPIC <#channel> [:<topic>]` | View/set topic |
| INVITE | `INVITE <user> <#channel>` | Invite user (op) |
| QUIT | `QUIT [:<message>]` | Disconnect |

## Channel Modes

- **+i** : Invite-only
- **+t** : Topic restricted to operators
- **+k** : Channel password (requires key parameter)
- **+o** : Operator privilege (requires user parameter)
- **+l** : User limit (requires limit parameter)

## Testing

```bash
# Run automated tests
./test_irc.sh

# Memory check
make valgrind

# Manual test with two clients
# Terminal 1:
nc localhost 6667
PASS test
NICK alice
USER alice 0 * :Alice
JOIN #test
PRIVMSG #test :Hello from alice

# Terminal 2:
nc localhost 6667
PASS test
NICK bob
USER bob 0 * :Bob
JOIN #test
# You should see alice's message
```

## Project Structure

```
.
├── Makefile
├── include/
│   ├── Server.hpp
│   ├── Client.hpp
│   ├── Channel.hpp
│   ├── Message.hpp
│   ├── CommandHandler.hpp
│   └── commands/
│       ├── PassCommand.hpp
│       ├── JoinCommand.hpp
│       ├── PartCommand.hpp
│       ├── PrivmsgCommand.hpp
│       ├── KickCommand.hpp
│       ├── ModeCommand.hpp
│       ├── TopicCommand.hpp
│       ├── InviteCommand.hpp
│       └── QuitCommand.hpp
├── src/
│   ├── main.cpp
│   ├── Server.cpp
│   ├── Client.cpp
│   ├── Channel.cpp
│   ├── Message.cpp
│   ├── CommandHandler.cpp
│   └── commands/
│       ├── PassCommand.cpp
│       ├── JoinCommand.cpp
│       ├── PartCommand.cpp
│       ├── PrivmsgCommand.cpp
│       ├── KickCommand.cpp
│       ├── ModeCommand.cpp
│       ├── TopicCommand.cpp
│       ├── InviteCommand.cpp
│       └── QuitCommand.cpp
└── test_irc.sh
```

## Technical Details

- **Language**: C++98 compliant
- **I/O**: Non-blocking sockets with poll() multiplexing
- **Memory**: Manual memory management (no smart pointers)
- **Architecture**: Command pattern for IRC commands
- **Protocol**: RFC 1459 compliant IRC protocol

## Build Requirements

- C++98 compatible compiler (g++, clang++)
- Make
- Netcat (for testing)

## Notes

- Maximum message length: 512 bytes (including \r\n)
- Channel names must start with '#' and be 1-50 characters
- Server uses port range 1024-65535
- All numeric replies follow IRC standard format
