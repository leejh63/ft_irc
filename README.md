*This project has been created as part of the 42 curriculum by Team MaumdaeRo.*

# ft_irc

## Description

`ft_irc` is an IRC server written in C++98.
It accepts multiple TCP clients, performs IRC registration, manages channels, and implements the required channel-operator commands.

The server uses a single `poll()`-based event loop. The listening socket and every client socket are configured as non-blocking file descriptors. Incoming data is buffered until a complete IRC line is available, and outgoing data is buffered until the socket is ready for writing.

## Instructions

### Build

Run `make` from the project root.

```bash
make
```

Available targets:

```bash
make
make clean
make fclean
make re
```

The project is compiled with:

```text
-Wall -Wextra -Werror -std=c++98
```

### Run

```bash
./ircserv <port> <password>
```

Example:

```bash
./ircserv 6667 pass123!
```

### Connect with `irssi`

```bash
irssi -c 127.0.0.1 -p 6667 -w pass123! -n tester
```

### Connect with `nc`

```bash
nc -C 127.0.0.1 6667
PASS pass123!
NICK alice
USER alice 0 * :Alice Example
JOIN #room
PRIVMSG #room :hello world
```

## Implemented Features

- IRC registration: `PASS`, `NICK`, `USER`
- Channel commands: `JOIN`, `PART`, `PRIVMSG`, `QUIT`
- Operator commands: `KICK`, `INVITE`, `TOPIC`, `MODE`
- Channel modes: `i`, `t`, `k`, `o`, `l`
- Client compatibility commands: `PING`, `PONG`, `CAP`, `WHO`
- Non-blocking socket handling with one `poll()` loop
- Buffered input and output handling for partial reads and writes
- Channel and client cleanup when a client leaves or disconnects

## Project Layout

```text
Makefile
README.md
include/
srcs/
docs/
  DESIGN.md
  COMMANDS.md
  MANUAL_CHECKS.md
  ARCHITECTURE.md
```

Additional notes are placed in `docs/`:

- `docs/DESIGN.md`: module responsibilities and server flow
- `docs/COMMANDS.md`: supported command behavior and channel modes
- `docs/MANUAL_CHECKS.md`: manual command flows for checking the server with raw IRC commands
- `docs/ARCHITECTURE.md`: in-depth architecture walkthrough with flow diagrams and a code-reading guide

## Resources

- RFC 1459: Internet Relay Chat Protocol
- RFC 2812: Internet Relay Chat Client Protocol
- Linux manual pages: `poll`, `socket`, `bind`, `listen`, `accept`, `recv`, `send`, `fcntl`
- `irssi` manual pages and client behavior

### AI usage

AI was used only for review support, documentation organization, and manual check scenario preparation. The implementation and final decisions were reviewed by the author.
