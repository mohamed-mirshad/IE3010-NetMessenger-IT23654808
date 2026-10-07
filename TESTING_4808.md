# NetMessenger Testing

Registration and username validation:
- Successful registration
- Duplicate username rejected with ERR 001

Messaging:
- LIST
- BCAST
- PMSG
- Invalid command rejected with ERR 005

Rooms:
- JOIN
- LEAVE
- ROOMS
- RMSG
- JOIN and LEAVE notifications
- Unknown room handling with ERR 003

File sharing:
- User-to-user file transfer
- Room file transfer
- Server-side file storage
- File size limit with ERR 004
- Unknown target handling with ERR 002

Concurrency and connection handling:
- Five simultaneous clients
- Graceful disconnect cleanup
- Multiple commands received over TCP
