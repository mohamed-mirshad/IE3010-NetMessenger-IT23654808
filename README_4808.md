# IE3010 NetMessenger

## Student Details

Registration Number: IT23654808
NID: 6548
Port: 10808

## Project

NetMessenger is a multi-client TCP/IP chat and file-sharing platform implemented in C using BSD sockets and POSIX threads.

## Build

make -f Makefile_4808

## Run Server

./server_4808.out

## Run Client

./client_4808.out

## Main Features

- User registration and presence
- User listing
- Broadcast messaging
- Private messaging
- Chat rooms
- Room join and leave notifications
- Room messaging
- User-to-user file sharing
- Room file sharing
- Server-side file storage
- File size validation
- Invalid command handling
- Timestamped server logging
- Multiple simultaneous clients
- Graceful client disconnection cleanup

## Project Files

- server_4808.c - server implementation
- client_4808.c - client implementation
- Makefile_4808 - build configuration
- netmsg_IT23654808.log - server activity log
- storage/IT23654808/ - received file storage
- TESTING_4808.md - testing evidence
- ARCHITECTURE_4808.md - architecture and concurrency
- PROTOCOL_4808.md - application protocol
