# NetMessenger Architecture

## Server Architecture

The NetMessenger server is implemented in C using BSD TCP sockets and
POSIX threads.

A main server thread creates the TCP listening socket on port 10808,
accepts incoming client connections, assigns a client slot, and creates
a dedicated thread for each connected client.

Each client thread handles registration, messaging, room operations,
file transfers, and client disconnection.

## Concurrency

POSIX pthreads are used because each client can be handled independently.
This allows multiple clients to communicate with the server concurrently.

Shared client and room data are protected using mutexes to reduce
concurrency-related conflicts.

## Client Architecture

The client uses a TCP socket to communicate with the server and a
receiver thread to continuously process incoming server messages while
the main thread accepts user commands.

## File Sharing

For SENDFILE, the client sends a command header followed immediately by
the specified number of raw file bytes. The server receives the exact
number of bytes, stores an unmodified copy under the user's storage
directory, and forwards the file to the intended user or room members.

## Protocol

Normal commands and server responses use newline-terminated text.
File payloads are transferred as raw bytes after the SENDFILE header.
