# NetMessenger Protocol

All normal protocol messages are newline terminated.

## Registration

REGISTER <username>

Successful response:
OK REGISTERED NID:6548

Duplicate username:
ERR 001 USERNAME_TAKEN NID:6548

## User and Messaging Commands

LIST

BCAST <message>

PMSG <username> <message>

## Room Commands

JOIN <room>

LEAVE <room>

ROOMS

RMSG <room> <message>

## File Transfer

SENDFILE <target> <filename> <bytes>

The SENDFILE command is followed immediately by exactly the specified
number of raw file bytes.

Successful sender response:

OK FILE_RECEIVED <filename> <bytes> NID:6548

File size errors:

ERR 004 FILE_TOO_LARGE NID:6548

Unknown user:

ERR 002 USER_NOT_FOUND NID:6548

Unknown room:

ERR 003 ROOM_NOT_FOUND NID:6548

Invalid command:

ERR 005 INVALID_COMMAND NID:6548

## Notifications

Broadcast:
MSG BCAST <sender> <message>

Private message:
MSG PRIV <sender> <message>

Room message:
MSG ROOM <room> <sender> <message>

Room join:
MSG JOIN <room> <username>

Room leave:
MSG LEAVE <room> <username>

File notification:
FILE <sender> <filename> <bytes>
