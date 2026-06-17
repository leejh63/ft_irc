*This project has been created as part of the 42 curriculum by jaeholee.*

# ft_irc

## Description

`ft_irc` is a small IRC server written in C++98 for the 42 `ft_irc` mandatory subject.
It runs as a single-process, non-blocking TCP server and uses one `poll()`-based event loop for accepting connections, reading client input, and flushing buffered output.

The project is organized around four clear layers:

- `Server`: socket lifecycle, `poll()`, buffered I/O, and connection management
- `IrcCore`: IRC command validation, registration flow, channel rules, and generated server actions
- `ClientRegistry` / `ChannelRegistry`: persistent client and channel state
- `IrcParser` / `IrcMessageBuilder`: IRC line parsing and wire-format message construction

A deeper explanation of the structure, data flow, and layer boundaries is available in [`ARCHITECTURE.md`](ARCHITECTURE.md).

## Implemented Features

### Mandatory IRC behavior

- password-based connection registration with `PASS`
- nickname registration and nickname change with `NICK`
- user registration with `USER`
- channel join / part with `JOIN` and `PART`
- private messaging to users and channels with `PRIVMSG`
- operator commands: `KICK`, `INVITE`, `TOPIC`, `MODE`
- channel modes: `i`, `t`, `k`, `o`, `l`
- minimal `QUIT`, `PING`, `PONG`, `CAP`, and `WHO` support for practical client compatibility

### Runtime behavior

- one `poll()` event loop for accept, read, and write
- non-blocking listening socket and non-blocking client sockets
- incremental line reconstruction for partial packet input
- buffered outgoing writes with `POLLOUT` enable/disable control
- disconnect propagation through a unified `QUIT` flow
- slow-client protection by disconnecting clients whose send queue exceeds the configured output-buffer limit

### Defensive edge-case handling

- a bad repeated `PASS` before final registration clears the previous password acceptance state
- nickname lookup and duplicate checks are canonicalized so case-only duplicates such as `alice` and `Alice` cannot coexist
- when the last channel operator leaves through `PART`, `QUIT`, disconnect, or `KICK`, one remaining member is automatically promoted to keep the channel manageable
- self-`KICK` no longer queues the same `KICK` message twice to the requester/target client
- empty trailing parameters used by commands such as `TOPIC #room :` are preserved

## Instructions

### Build

Run `make` from the project root, where this `README.md` and the `Makefile` are located.

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

### Reference client

The practical reference client used during local verification is `irssi`.

```bash
irssi -c 127.0.0.1 -p 6667 -w pass123! -n tester
```

### Quick manual test

You can also verify the server with `nc`:

```bash
nc -C 127.0.0.1 6667
PASS pass123!
NICK alice
USER alice 0 * :Alice Example
JOIN #room
PRIVMSG #room :hello world
```

## Project Layout

```text
Makefile
README.md
ARCHITECTURE.md
FIX_VERIFICATION.md
include/
  Server.hpp
  SocketMonitor.hpp
  Fd.hpp
  ClientEntry.hpp
  ClientRegistry.hpp
  ChannelEntry.hpp
  ChannelRegistry.hpp
  IrcCommand.hpp
  ServerAction.hpp
  IrcParser.hpp
  IrcCore.hpp
  IrcMessageBuilder.hpp
  IrcServerInfo.hpp
srcs/
  Server.cpp
  SocketMonitor.cpp
  Fd.cpp
  ClientRegistry.cpp
  ChannelRegistry.cpp
  IrcParser.cpp
  IrcCore.cpp
  IrcCoreSupport.cpp
  IrcCoreRegistration.cpp
  IrcCoreProtocol.cpp
  IrcCoreChannel.cpp
  IrcMessageBuilder.cpp
  IrcServerInfo.cpp
  Signal.cpp
  Utils.cpp
  main.cpp
```

## Design Notes

- Transport logic and IRC domain rules are separated.
- `IrcCore` does not call `send()` or `close()` directly; it produces `ServerAction` objects that `Server` executes.
- Client transport state and IRC registration state are intentionally unified into one `ClientEntry` to keep the project readable at this size.
- Channel membership, operator state, invite state, topic, key, and limit are centralized in `ChannelRegistry`.
- `SocketMonitor` isolates `pollfd` storage from the rest of the server loop.

## Documentation

- High-level architecture and flow: [`ARCHITECTURE.md`](ARCHITECTURE.md)
- Fix verification notes: [`FIX_VERIFICATION.md`](FIX_VERIFICATION.md)
- Project subject: `ft_irc.pdf`

## Resources

- RFC 1459: Internet Relay Chat Protocol
- RFC 2812: Internet Relay Chat Client Protocol
- `man 2 poll`
- `man 2 socket`
- `man 2 recv`
- `man 2 send`
- `man 2 fcntl`

## AI Usage

AI was used as a review and refactoring assistant for:

- checking the mandatory subject against the implementation
- evaluating structure, naming, and layer boundaries
- identifying edge cases in registration, channel operator lifecycle, nickname matching, and disconnect handling
- improving readability without changing the intended architecture

All suggestions were manually reviewed, adapted to the project, and tested locally.
