# Reflection

## IE3010 Network Programming – NetMessenger

**Registration Number:** IT23654808
**NID:** 6548
**Server Port:** 10808

## Introduction

The IE3010 Network Programming assignment provided practical experience in developing a multi-client TCP/IP communication application using the C programming language. The NetMessenger project helped me apply networking concepts learned during the module to a working client-server application.

## Knowledge and Skills Gained

Through this assignment, I gained a better understanding of BSD socket programming and the communication process between TCP clients and a server. I worked with socket creation, connection establishment, binding, listening, accepting client connections, and sending and receiving data.

I also gained practical experience with POSIX threads. The server creates a separate thread for each connected client, allowing multiple clients to communicate with the server at the same time.

Another important concept I learned was TCP stream handling. A single `recv()` operation does not always contain exactly one complete command. Therefore, persistent input buffers were used to store received data and process complete newline-terminated commands.

## Implementation Experience

The NetMessenger application includes user registration, user listing, broadcast messaging, private messaging, chat rooms, room messaging, and file sharing.

The room functionality helped me understand how shared data structures can be used to maintain room membership. The server maintains client and room information and uses mutexes to protect shared resources accessed by multiple threads.

The file-sharing functionality was another important part of the implementation. Files can be sent to individual users or room members through the server. The implementation also validates the file size and stores received files under the registration-number-specific storage directory.

## Challenges Faced

One of the main challenges was handling multiple clients concurrently. Since several client threads can access shared client and room information, incorrect synchronization could cause inconsistent data or unexpected behaviour.

Another challenge was handling TCP data correctly. Data received from the network may contain partial commands or multiple commands in one `recv()` call. Using an input buffer and processing complete lines helped address this issue.

File transfer was also challenging because the server needs to distinguish between command data and binary file data while maintaining the TCP stream correctly.

## How the Challenges Were Addressed

The concurrency issue was addressed using POSIX mutexes for shared client and room data. The server also performs cleanup when a client disconnects by removing the client from rooms and clearing the client slot.

For TCP stream handling, persistent buffers were implemented so that incomplete data could remain in the buffer until the remaining data arrived.

For file transfer, the application uses a file header containing the sender, filename, and file size. The server then receives the specified number of file bytes, stores the file, and forwards it to the intended user or room members.

## Overall Learning

This assignment improved my practical understanding of network programming beyond the theoretical concepts discussed in lectures. I learned that developing a network application requires careful handling of TCP streams, concurrent clients, shared resources, error conditions, and data integrity.

The assignment also improved my debugging and testing skills because the application had to be tested with different users, commands, rooms, and file-transfer scenarios.

## Conclusion

Overall, the NetMessenger assignment was a valuable practical experience. It helped me understand how a multi-client TCP server can be designed using C, BSD sockets, POSIX threads, mutexes, and structured application-level commands. The project also improved my ability to analyse networking problems, debug implementations, and build a functional network application.
