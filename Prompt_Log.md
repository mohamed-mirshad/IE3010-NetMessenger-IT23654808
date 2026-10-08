# Prompt Log

## IE3010 Network Programming – NetMessenger

**Registration Number:** IT23654808
**NID:** 6548
**Server Port:** 10808

### Purpose

This prompt log records the use of AI assistance during the development, debugging, testing, and documentation of the NetMessenger application. AI assistance was used as a support tool to understand programming concepts, identify possible issues, and improve the implementation.

### Prompt 1 – TCP Socket Programming

**Prompt:**
Asked for guidance on implementing a TCP client-server chat application in C using BSD sockets.

**Purpose:**
To understand the basic socket creation, binding, listening, accepting, connecting, sending, and receiving process.

**Use in the project:**
The guidance helped in understanding the overall client-server communication structure.

### Prompt 2 – Client-Server Communication

**Prompt:**
Asked for assistance with implementing communication between multiple clients and a server using TCP sockets.

**Purpose:**
To understand how multiple clients can connect to one server and exchange messages.

**Use in the project:**
The server was implemented using POSIX threads so that multiple connected clients could be handled concurrently.

### Prompt 3 – Registration and User Management

**Prompt:**
Asked for guidance on implementing user registration and checking whether a username is already registered.

**Purpose:**
To understand how connected clients can be assigned usernames and how duplicate usernames can be detected.

**Use in the project:**
The server maintains a client array containing socket descriptors, usernames, and registration status.

### Prompt 4 – Chat Rooms

**Prompt:**
Asked for assistance with implementing room creation, joining, leaving, room listing, and room messaging.

**Purpose:**
To understand how users can be grouped into chat rooms and how messages can be distributed to room members.

**Use in the project:**
The server maintains room structures and member lists and implements JOIN, ROOMS, LEAVE, and RMSG commands.

### Prompt 5 – TCP Input Buffering

**Prompt:**
Asked for help understanding how TCP data can arrive in partial or multiple messages and how an input buffer can be used to process complete commands.

**Purpose:**
To handle the fact that one `recv()` call does not necessarily correspond to one complete application command.

**Use in the project:**
Persistent input buffers were implemented on the server and client to process complete newline-terminated commands.

### Prompt 6 – File Transfer

**Prompt:**
Asked for guidance on implementing file transfer between clients through the server using TCP sockets.

**Purpose:**
To understand how file headers and file data can be transferred reliably.

**Use in the project:**
The application implements the `SENDFILE` command, file-size validation, server-side storage, and forwarding of files to users or room members.

### Prompt 7 – Thread Synchronization

**Prompt:**
Asked for assistance with protecting shared client and room data when multiple threads access them simultaneously.

**Purpose:**
To understand the use of POSIX mutexes for concurrent access.

**Use in the project:**
`clients_mutex` and `rooms_mutex` are used to protect shared client and room structures.

### Prompt 8 – Logging and Error Handling

**Prompt:**
Asked for guidance on implementing server-side logging and handling invalid commands and client disconnections.

**Purpose:**
To improve monitoring and error handling in the application.

**Use in the project:**
The server records timestamped events in `netmsg_IT23654808.log` and returns protocol error responses for invalid operations.

### Prompt 9 – Testing and Debugging

**Prompt:**
Asked for assistance with testing the NetMessenger client and server and identifying possible implementation issues.

**Purpose:**
To verify the implemented features and improve reliability.

**Use in the project:**
The client-server application was tested using multiple client instances and different messaging, room, and file-sharing operations.

### Note

AI assistance was used for learning, guidance, debugging, and documentation support. The final implementation was reviewed and adapted to meet the requirements of the IE3010 Network Programming assignment.

