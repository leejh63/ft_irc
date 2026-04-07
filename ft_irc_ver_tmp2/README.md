*This project has been created as part of the 42 curriculum by jaeholee.*

# ft_irc

## Overview

`ft_irc` is a small IRC server written in C++98.
It uses a single-process, non-blocking TCP server with one `poll()`-based event loop,
and implements the mandatory IRC features required by the 42 `ft_irc` subject.

The codebase is intentionally organized around a small number of clear layers:

- `Server`: socket lifecycle, `poll()`, buffered I/O, connection management
- `IrcCore`: IRC command handling, validation, domain rules, and produced server actions
- `ClientRegistry` / `ChannelRegistry`: persistent client and channel state
- `IrcParser` / `IrcMessageBuilder`: protocol parsing and reply/message construction

A deeper explanation of the full structure, data flow, and layer boundaries is available in [ARCHITECTURE.md](ARCHITECTURE.md).

## Implemented Features

### Mandatory IRC behavior

- password-based connection registration with `PASS`
- nickname registration with `NICK`
- user registration with `USER`
- channel join / part with `JOIN` and `PART`
- private messaging to users and channels with `PRIVMSG`
- operator commands: `KICK`, `INVITE`, `TOPIC`, `MODE`
- channel modes: `i`, `t`, `k`, `o`, `l`

### Transport / runtime behavior

- one `poll()` event loop for accept, read, and write
- non-blocking listening socket and non-blocking client sockets
- incremental line reconstruction for partial packet input
- buffered outgoing writes with `POLLOUT` enable/disable control
- disconnect propagation through a unified `QUIT` flow
- slow-client protection by disconnecting clients whose send queue exceeds the configured output-buffer limit

## Build

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

## Run

```bash
./ircserv <port> <password>
```

Example:

```bash
./ircserv 6667 pass123!
```

## Reference Client

The practical reference client used during local verification is `irssi`.

Example:

```bash
irssi -c 127.0.0.1 -p 6667 -w pass123! -n tester
```

## Quick Manual Test

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

- The server keeps transport logic and IRC rules separated.
- `IrcCore` does not call `send()` or `close()` directly.
  It produces `ServerAction` objects that `Server` executes.
- Client transport state and IRC registration state were intentionally unified into one `ClientEntry` to keep the project readable at this size.
- `SocketMonitor` isolates `pollfd` storage and makes a future `epoll` migration more realistic.

## Documentation

- High-level overview and full architecture: [ARCHITECTURE.md](ARCHITECTURE.md)
- Project subject: `ft_irc.pdf`

## Resources

- RFC 1459: Internet Relay Chat Protocol
- RFC 2812: Internet Relay Chat Client Protocol
- `man 2 poll`
- `man 2 socket`
- `man 2 recv`
- `man 2 send`

## AI Usage

AI was used as a review and refactoring assistant for:

- checking the mandatory subject against the current implementation
- evaluating structure, naming, and layer boundaries
- identifying edge cases in disconnect handling and server lifetime
- improving readability without overengineering the codebase

All suggestions were manually reviewed, adapted to the project, and tested locally.
