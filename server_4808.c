#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>

#define PORT 10808
#define BACKLOG 5
#define MAX_CLIENTS 100
#define USERNAME_LEN 50

#define MAX_ROOMS 50
#define ROOM_NAME_LEN 50

void log_event(const char *event)
{
    FILE *log_file = fopen("netmsg_IT23654808.log", "a");

    if (log_file == NULL)
    {
        return;
    }

    time_t now = time(NULL);
    struct tm *current_time = localtime(&now);

    if (current_time != NULL)
    {
        fprintf(
            log_file,
            "[%04d-%02d-%02d %02d:%02d:%02d] %s\n",
            current_time->tm_year + 1900,
            current_time->tm_mon + 1,
            current_time->tm_mday,
            current_time->tm_hour,
            current_time->tm_min,
            current_time->tm_sec,
            event);
    }

    fclose(log_file);
}

typedef struct
{
    int socket_fd;
    char username[USERNAME_LEN];
    int registered;
} Client;

typedef struct
{
    char name[ROOM_NAME_LEN];
    int members[MAX_CLIENTS];
    int member_count;
} Room;

typedef struct
{
    int client_index;
} ThreadData;

Client clients[MAX_CLIENTS];
Room rooms[MAX_ROOMS];

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t rooms_mutex = PTHREAD_MUTEX_INITIALIZER;


/* FIND ROOM */
int find_room(const char *room_name)
{
    int i;

    for (i = 0; i < MAX_ROOMS; i++)
    {
        if (rooms[i].name[0] != '\0' &&
            strcmp(rooms[i].name, room_name) == 0)
        {
            return i;
        }
    }

    return -1;
}


/* CLEANUP DISCONNECTED CLIENT */
void cleanup_client(int client_index)
{
    int i;
    int j;

    pthread_mutex_lock(&rooms_mutex);

    /* Remove client from all rooms */
    for (i = 0; i < MAX_ROOMS; i++)
    {
        if (rooms[i].name[0] != '\0')
        {
            for (j = 0; j < rooms[i].member_count; j++)
            {
                if (rooms[i].members[j] == client_index)
                {
                    int k;

                    for (k = j;
                         k < rooms[i].member_count - 1;
                         k++)
                    {
                        rooms[i].members[k] =
                            rooms[i].members[k + 1];
                    }

                    rooms[i].member_count--;
                    j--;
                }
            }
        }
    }

    pthread_mutex_unlock(&rooms_mutex);

    /* Clear client slot */
    pthread_mutex_lock(&clients_mutex);

    clients[client_index].socket_fd = 0;
    clients[client_index].username[0] = '\0';
    clients[client_index].registered = 0;

    pthread_mutex_unlock(&clients_mutex);

    printf("Client slot %d cleaned up.\n", client_index);
}


/* ADD CLIENT */
int add_client(int client_fd)
{
    int i;

    pthread_mutex_lock(&clients_mutex);

    for (i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].socket_fd == 0)
        {
            clients[i].socket_fd = client_fd;
            clients[i].registered = 0;
            clients[i].username[0] = '\0';

            pthread_mutex_unlock(&clients_mutex);

            return i;
        }
    }

    pthread_mutex_unlock(&clients_mutex);

    return -1;
}

int send_all(int fd, const void *data, int length)
{
    int total_sent = 0;

    while (total_sent < length)
    {
        int sent = send(fd,
                        (const char *)data + total_sent,
                        length - total_sent,
                        0);

        if (sent <= 0)
        {
            return -1;
        }

        total_sent += sent;
    }

    return 0;
}

/* CLIENT THREAD */
void *handle_client(void *arg)
{
    ThreadData *data = (ThreadData *)arg;

    int client_index = data->client_index;

    free(data);

    int client_fd = clients[client_index].socket_fd;

    printf("Client handled by thread.\n");


    /* SEND WELCOME MESSAGE */
    char *message = "WELCOME TO NETMESSENGER\n";

    if (send(client_fd,
             message,
             strlen(message),
             0) < 0)
    {
        perror("send welcome");
    }
    else
    {
        printf("Welcome message sent.\n");
    }


    /*
     * TCP INPUT BUFFER
     *
     * recv() does not guarantee one command per call.
     * input_buffer stores incomplete/multiple commands.
     */
    char buffer[1024];
    char input_buffer[4096];

    int input_length = 0;
    int bytes_received;

    memset(input_buffer, 0, sizeof(input_buffer));


    /* RECEIVE LOOP */
    while (1)
    {
        bytes_received = recv(client_fd,
                              buffer,
                              sizeof(buffer) - 1,
                              0);

        /* RECEIVE ERROR */
        if (bytes_received < 0)
        {
            perror("recv");

            cleanup_client(client_index);

            break;
        }


        /* CLIENT DISCONNECTED */
        if (bytes_received == 0)
        {
            printf("Client disconnected.\n");
	    log_event("Client disconnected");
            cleanup_client(client_index);

            break;
        }


        buffer[bytes_received] = '\0';


        /*
         * Add received bytes to persistent input buffer.
         */
        if (input_length + bytes_received >=
            sizeof(input_buffer))
        {
            printf("Input buffer full.\n");

            input_length = 0;

            memset(input_buffer,
                   0,
                   sizeof(input_buffer));

            continue;
        }


        memcpy(input_buffer + input_length,
               buffer,
               bytes_received);

        input_length += bytes_received;

        input_buffer[input_length] = '\0';


        printf("Received data: %s", input_buffer);


        /*
         * Process every complete line currently
         * available in the input buffer.
         */
        char *newline_pos;

        while ((newline_pos =
                strchr(input_buffer, '\n')) != NULL)
        {
            int command_length;

            command_length =
                newline_pos - input_buffer;


            /* Command too long */
            if (command_length >= sizeof(buffer))
            {
                printf("Command too long.\n");

                memmove(input_buffer,
                        newline_pos + 1,
                        input_length -
                        command_length -
                        1);

                input_length -= command_length + 1;

                input_buffer[input_length] = '\0';

                continue;
            }


            /* Copy one complete command */
            memcpy(buffer,
                   input_buffer,
                   command_length);

            buffer[command_length] = '\0';


            /* Remove processed command */
            memmove(input_buffer,
                    newline_pos + 1,
                    input_length -
                    command_length -
                    1);

            input_length -= command_length + 1;

            input_buffer[input_length] = '\0';


            
	    printf("Processing command: %s\n", buffer);
            char log_message[1200];

	   snprintf(
    		log_message,
    		sizeof(log_message),
    		"Command: %s",
    		buffer);

	    log_event(log_message);
	    
	    int known_command = 0;

            /* =========================================
             * REGISTER
             * ========================================= */
            if (strncmp(buffer, "REGISTER ", 9) == 0)
            {
                 known_command = 1;
                 char username[USERNAME_LEN];

                sscanf(buffer + 9,
                       "%49s",
                       username);

                int username_taken = 0;
                int i;

                pthread_mutex_lock(&clients_mutex);

                for (i = 0;
                     i < MAX_CLIENTS;
                     i++)
                {
                    if (clients[i].registered &&
                        strcmp(clients[i].username,
                               username) == 0)
                    {
                        username_taken = 1;
                        break;
                    }
                }


                if (!username_taken)
                {
                    strcpy(clients[client_index].username,
                           username);

                    clients[client_index].registered = 1;
                }

                pthread_mutex_unlock(&clients_mutex);


                char response[100];

                if (username_taken)
                {
                    snprintf(response,
                             sizeof(response),
                             "ERR 001 USERNAME_TAKEN NID:6548\n");
                }
                else
                {
                    snprintf(response,
                             sizeof(response),
                             "OK REGISTERED NID:6548\n");
                }


                send(client_fd,
                     response,
                     strlen(response),
                     0);

                printf("REGISTER response sent: %s",
                       response);
            }


            /* =========================================
             * LIST USERS
             * ========================================= */
            if (strncmp(buffer, "LIST", 4) == 0)
            {
                known_command = 1;
                 char response[1024];

                int offset = 0;
                int i;

                pthread_mutex_lock(&clients_mutex);

                offset += snprintf(
                    response + offset,
                    sizeof(response) - offset,
                    "USERS");

                for (i = 0;
                     i < MAX_CLIENTS;
                     i++)
                {
                    if (clients[i].registered)
                    {
                        offset += snprintf(
                            response + offset,
                            sizeof(response) - offset,
                            " %s",
                            clients[i].username);
                    }
                }

                pthread_mutex_unlock(&clients_mutex);


                offset += snprintf(
                    response + offset,
                    sizeof(response) - offset,
                    " NID:6548\n");


                send(client_fd,
                     response,
                     strlen(response),
                     0);
            }


            /* =========================================
             * BROADCAST MESSAGE
             * ========================================= */
            if (strncmp(buffer, "BCAST ", 6) == 0)
            {
                known_command = 1;
                char message[1024];
                char response[1024];

                int i;

                strcpy(message,
                       buffer + 6);


                pthread_mutex_lock(&clients_mutex);

                for (i = 0;
                     i < MAX_CLIENTS;
                     i++)
                {
                    if (clients[i].registered &&
                        clients[i].socket_fd != client_fd)
                    {
                        snprintf(
                            response,
                            sizeof(response),
                            "MSG BCAST %s %s\n",
                            clients[client_index].username,
                            message);


                        send(clients[i].socket_fd,
                             response,
                             strlen(response),
                             0);
                    }
                }

                pthread_mutex_unlock(&clients_mutex);


                snprintf(response,
                         sizeof(response),
                         "OK SENT NID:6548\n");


                send(client_fd,
                     response,
                     strlen(response),
                     0);
            }


            /* =========================================
             * PRIVATE MESSAGE
             * ========================================= */
            if (strncmp(buffer, "PMSG ", 5) == 0)
            {
                known_command = 1;
                char target[USERNAME_LEN];
                char message[1024];
                char response[1024];

                int i;
                int target_found = 0;


                sscanf(buffer + 5,
                       "%49s %[^\n]",
                       target,
                       message);


                pthread_mutex_lock(&clients_mutex);


                for (i = 0;
                     i < MAX_CLIENTS;
                     i++)
                {
                    if (clients[i].registered &&
                        strcmp(clients[i].username,
                               target) == 0)
                    {
                        target_found = 1;


                        snprintf(
                            response,
                            sizeof(response),
                            "MSG PRIV %s %s\n",
                            clients[client_index].username,
                            message);


                        send(clients[i].socket_fd,
                             response,
                             strlen(response),
                             0);

                        break;
                    }
                }


                pthread_mutex_unlock(&clients_mutex);


                if (target_found)
                {
                    snprintf(
                        response,
                        sizeof(response),
                        "OK SENT NID:6548\n");
                }
                else
                {
                    snprintf(
                        response,
                        sizeof(response),
                        "ERR 002 USER_NOT_FOUND NID:6548\n");
                }


                send(client_fd,
                     response,
                     strlen(response),
                     0);
            }


            /* =========================================
             * JOIN ROOM
             * ========================================= */
            if (strncmp(buffer, "JOIN ", 5) == 0)
            {
                known_command = 1;
                char room_name[ROOM_NAME_LEN];
                char response[1024];

                int room_index;
                int i;


                sscanf(buffer + 5,
                       "%49s",
                       room_name);


                pthread_mutex_lock(&rooms_mutex);


                room_index =
                    find_room(room_name);


                /* Create room if absent */
                if (room_index == -1)
                {
                    for (i = 0;
                         i < MAX_ROOMS;
                         i++)
                    {
                        if (rooms[i].name[0] == '\0')
                        {
                            strcpy(rooms[i].name,
                                   room_name);

                            rooms[i].member_count = 0;

                            room_index = i;

                            break;
                        }
                    }
                }


                /* Add client */
                if (room_index != -1)
                {
                    int already_member = 0;


                    for (i = 0;
                         i < rooms[room_index].member_count;
                         i++)
                    {
                        if (rooms[room_index].members[i]
                            == client_index)
                        {
                            already_member = 1;
                            break;
                        }
                    }


                    if (!already_member &&
                        rooms[room_index].member_count
                        < MAX_CLIENTS)
                    {
                        rooms[room_index].members[
                            rooms[room_index].member_count]
                            = client_index;

                        rooms[room_index].member_count++;
                    }
                }


                pthread_mutex_unlock(&rooms_mutex);


		if (room_index == -1)
{
    snprintf(
        response,
        sizeof(response),
        "ERR 005 SERVER_FULL NID:6548\n");
}
else
{
    snprintf(
        response,
        sizeof(response),
        "OK JOINED %s NID:6548\n",
        room_name);

    send(client_fd,
         response,
         strlen(response),
         0);

    /* Notify existing room members about the new user */
    char join_message[1024];

    snprintf(
        join_message,
        sizeof(join_message),
        "MSG JOIN %s %s NID:6548\n",
        room_name,
        clients[client_index].username);

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (i != client_index &&
            clients[i].registered)
        {
            int is_member = 0;

            for (int j = 0;
                 j < rooms[room_index].member_count;
                 j++)
            {
                if (rooms[room_index].members[j] == i)
                {
                    is_member = 1;
                    break;
                }
            }

            if (is_member)
            {
                send(clients[i].socket_fd,
                     join_message,
                     strlen(join_message),
                     0);
            }
        }
    }

    continue;
}

                send(client_fd,
                     response,
                     strlen(response),
                     0);
            }


            /* =========================================
             * LIST ROOMS
             * ========================================= */
            if (strncmp(buffer, "ROOMS", 5) == 0)
            {
                known_command = 1;
                char response[1024];

                int offset = 0;
                int i;


                pthread_mutex_lock(&rooms_mutex);


                offset += snprintf(
                    response + offset,
                    sizeof(response) - offset,
                    "ROOMS");


                for (i = 0;
                     i < MAX_ROOMS;
                     i++)
                {
                    if (rooms[i].name[0] != '\0')
                    {
                        offset += snprintf(
                            response + offset,
                            sizeof(response) - offset,
                            " %s",
                            rooms[i].name);
                    }
                }


                pthread_mutex_unlock(&rooms_mutex);


                offset += snprintf(
                    response + offset,
                    sizeof(response) - offset,
                    " NID:6548\n");


                send(client_fd,
                     response,
                     strlen(response),
                     0);
            }


            /* =========================================
             * LEAVE ROOM
             * ========================================= */
            if (strncmp(buffer, "LEAVE ", 6) == 0)
            {
                known_command = 1;
                char room_name[ROOM_NAME_LEN];
                char response[1024];

                int room_index;
                int i;


                sscanf(buffer + 6,
                       "%49s",
                       room_name);


                pthread_mutex_lock(&rooms_mutex);


                room_index =
                    find_room(room_name);

 		  if (room_index != -1)
{
    /* Notify other room members that this user is leaving */
    char leave_message[1024];

    snprintf(
        leave_message,
        sizeof(leave_message),
        "MSG LEAVE %s %s NID:6548\n",
        room_name,
        clients[client_index].username);

    for (int k = 0; k < rooms[room_index].member_count; k++)
    {
        int member_index = rooms[room_index].members[k];

        if (member_index != client_index &&
            clients[member_index].registered)
        {
            send(
                clients[member_index].socket_fd,
                leave_message,
                strlen(leave_message),
                0);
        }
    }

    for (i = 0;
         i < rooms[room_index].member_count;
         i++)


                    {
                        if (rooms[room_index].members[i]
                            == client_index)
                        {
                            int j;


                            for (j = i;
                                 j < rooms[room_index].member_count - 1;
                                 j++)
                            {
                                rooms[room_index].members[j] =
                                    rooms[room_index].members[j + 1];
                            }


                            rooms[room_index].member_count--;

                            break;
                        }
                    }
                }


                pthread_mutex_unlock(&rooms_mutex);


                if (room_index == -1)
                {
                    snprintf(
                        response,
                        sizeof(response),
                        "ERR 003 ROOM_NOT_FOUND NID:6548\n");
                }
                else
                {
                    snprintf(
                        response,
                        sizeof(response),
                        "OK LEFT %s NID:6548\n",
                        room_name);
                }


                send(client_fd,
                     response,
                     strlen(response),
                     0);
            }

            /* =========================================
             * SEND FILE
             * ========================================= */
            if (strncmp(buffer, "SENDFILE ", 9) == 0)
            {
                known_command = 1;
                char target[USERNAME_LEN];
                char filename[256];
                long file_size;

                char response[1024];
                char filepath[512];

                int target_client = -1;
                int target_room = -1;
                int is_room = 0;
                int i;

                /*
                 * Format:
                 * SENDFILE <target> <filename> <bytes>
                 */
                if (sscanf(buffer + 9,
                           "%49s %255s %ld",
                           target,
                           filename,
                           &file_size) != 3 ||
                    file_size < 0)
                {
                    snprintf(response,
                             sizeof(response),
                             "ERR 005 INVALID_FILE_COMMAND NID:6548\n");

                    send(client_fd,
                         response,
                         strlen(response),
                         0);

                    continue;
                }

                /*
                 * Check file size.
                 * Maximum allowed file size = 5 MB.
                 */
                if (file_size > 5 * 1024 * 1024)
                {
                    snprintf(response,
                             sizeof(response),
                             "ERR 004 FILE_TOO_LARGE NID:6548\n");

                    send(client_fd,
                         response,
                         strlen(response),
                         0);

                    continue;
                }

                /*
                 * Find target user.
                 */
                pthread_mutex_lock(&clients_mutex);

                for (i = 0;
                     i < MAX_CLIENTS;
                     i++)
                {
                    if (clients[i].registered &&
                        strcmp(clients[i].username,
                               target) == 0)
                    {
                        target_client = i;
                        break;
                    }
                }

                pthread_mutex_unlock(&clients_mutex);

                /*
                 * If not a user, check whether it is a room.
                 */
                if (target_client == -1)
                {
                    pthread_mutex_lock(&rooms_mutex);

                    target_room = find_room(target);

                    pthread_mutex_unlock(&rooms_mutex);

                    if (target_room != -1)
                    {
                        is_room = 1;
                    }
                }

                /*
                 * Unknown target.
                 */
		if (target_client == -1 &&
    !is_room)
{
    /*
     * Discard the exact file bytes already sent
     * by the client so the TCP stream stays synchronized.
     */
    long discarded = 0;
    char discard_buffer[1024];

    while (discarded < file_size)
    {
        long remaining = file_size - discarded;

        int to_receive =
            remaining < sizeof(discard_buffer)
            ? (int)remaining
            : sizeof(discard_buffer);

        int received =
            recv(client_fd,
                 discard_buffer,
                 to_receive,
                 0);

        if (received <= 0)
        {
            cleanup_client(client_index);
            return NULL;
        }

        discarded += received;
    }

    snprintf(response,
             sizeof(response),
             "ERR 002 USER_NOT_FOUND NID:6548\n");

    send(client_fd,
         response,
         strlen(response),
         0);

    continue;
}
                /*
                 * Create storage directory if necessary.
                 */
                mkdir("./storage", 0777);
                mkdir("./storage/IT23654808", 0777);

                snprintf(filepath,
                         sizeof(filepath),
                         "./storage/IT23654808/%s",
                         filename);

                FILE *file =
                    fopen(filepath, "wb");

                if (file == NULL)
                {
                    perror("fopen");

                    snprintf(response,
                             sizeof(response),
                             "ERR 005 FILE_STORAGE_ERROR NID:6548\n");

                    send(client_fd,
                         response,
                         strlen(response),
                         0);

                    continue;
                }

                /*
                 * Receive exactly file_size bytes.
                 *
                 * First consume bytes already present
                 * in input_buffer.
                 */
                long remaining = file_size;

                while (remaining > 0)
                {
                    if (input_length > 0)
                    {
                        int available = input_length;

                        int to_write =
                            (remaining < available)
                            ? (int)remaining
                            : available;

                        fwrite(input_buffer,
                               1,
                               to_write,
                               file);

                        memmove(input_buffer,
                                input_buffer + to_write,
                                input_length - to_write);

                        input_length -= to_write;

                        input_buffer[input_length] = '\0';

                        remaining -= to_write;
                    }
                    else
{
    unsigned char file_buffer[1024];

    int received =
        recv(client_fd,
             file_buffer,
             sizeof(file_buffer),
             0);

    if (received <= 0)
    {
        fclose(file);
        cleanup_client(client_index);
        return NULL;
    }

    int to_write =
        (remaining < received)
        ? (int)remaining
        : received;

    fwrite(file_buffer,
           1,
           to_write,
           file);

    remaining -= to_write;

    if (received > to_write)
    {
        int extra_bytes = received - to_write;

        memcpy(input_buffer,
               file_buffer + to_write,
               extra_bytes);

        input_length = extra_bytes;
        input_buffer[input_length] = '\0';
    }
}


                }

                fclose(file);

                /*
                 * Tell sender that file was received.
                 */
                snprintf(response,
                         sizeof(response),
                         "OK FILE_RECEIVED %s %ld NID:6548\n",
                         filename,
                         file_size);

                send(client_fd,
                     response,
                     strlen(response),
                     0);

                /*
                 * Re-open stored file for forwarding.
                 */
                file = fopen(filepath, "rb");

                if (file == NULL)
                {
                    continue;
                }

                /*
                 * Send file to a single user.
                 */
                if (target_client != -1)
                {
                    snprintf(response,
                             sizeof(response),
                             "FILE %s %s %ld\n",
                             clients[client_index].username,
                             filename,
                             file_size);

                    send_all(clients[target_client].socket_fd,
                             response,
                             strlen(response));

                    char file_buffer[1024];

                    size_t bytes_read;

                    while ((bytes_read =
                            fread(file_buffer,
                                  1,
                                  sizeof(file_buffer),
                                  file)) > 0)
                    {
                        send_all(clients[target_client].socket_fd,
                                 file_buffer,
                                 bytes_read);
                    }
                }

                /*
                 * Send file to all other members of a room.
                 */
                if (is_room)
                {
                    int room_members[MAX_CLIENTS];
                    int member_count = 0;

                    pthread_mutex_lock(&rooms_mutex);

                    for (i = 0;
                         i < rooms[target_room].member_count;
                         i++)
                    {
                        int member_index =
                            rooms[target_room].members[i];

                        if (member_index != client_index &&
                            clients[member_index].registered)
                        {
                            room_members[member_count] =
                                member_index;

                            member_count++;
                        }
                    }

                    pthread_mutex_unlock(&rooms_mutex);

                    for (i = 0;
                         i < member_count;
                         i++)
                    {
                        rewind(file);

                        snprintf(response,
                                 sizeof(response),
                                 "FILE %s %s %ld\n",
                                 clients[client_index].username,
                                 filename,
                                 file_size);

                        send_all(
                            clients[room_members[i]].socket_fd,
                            response,
                            strlen(response));

                        char file_buffer[1024];

                        size_t bytes_read;

                        while ((bytes_read =
                                fread(file_buffer,
                                      1,
                                      sizeof(file_buffer),
                                      file)) > 0)
                        {
                            send_all(
                                clients[room_members[i]].socket_fd,
                                file_buffer,
                                bytes_read);
                        }
                    }
                }

                fclose(file);

                printf("File received and forwarded: %s (%ld bytes)\n",
                       filename,
                       file_size);
            	char file_log[512];

		snprintf(
    			file_log,
    			sizeof(file_log),
			"File received and forwarded: %s (%ld bytes)",
    			filename,
    			file_size);

		log_event(file_log);
		}


            /* =========================================
             * ROOM MESSAGE
             * ========================================= */
            if (strncmp(buffer, "RMSG ", 5) == 0)
            {
 		known_command = 1;
                char room_name[ROOM_NAME_LEN];
                char message[1024];
                char response[1024];

                int room_index;
                int i;
                int sender_in_room = 0;


                sscanf(buffer + 5,
                       "%49s %[^\n]",
                       room_name,
                       message);


                pthread_mutex_lock(&rooms_mutex);


                room_index =
                    find_room(room_name);


                if (room_index != -1)
                {
                    /* Check sender membership */
                    for (i = 0;
                         i < rooms[room_index].member_count;
                         i++)
                    {
                        if (rooms[room_index].members[i]
                            == client_index)
                        {
                            sender_in_room = 1;
                            break;
                        }
                    }


                    /* Send to other room members */
                    if (sender_in_room)
                    {
                        for (i = 0;
                             i < rooms[room_index].member_count;
                             i++)
                        {
                            int member_index =
                                rooms[room_index].members[i];


                            if (member_index != client_index &&
                                clients[member_index].registered)
                            {
                                snprintf(
                                    response,
                                    sizeof(response),
                                    "MSG ROOM %s %s %s\n",
                                    room_name,
                                    clients[client_index].username,
                                    message);


                                send(
                                    clients[member_index].socket_fd,
                                    response,
                                    strlen(response),
                                    0);
                            }
                        }
                    }
                }


                pthread_mutex_unlock(&rooms_mutex);


                if (room_index == -1)
                {
                    snprintf(
                        response,
                        sizeof(response),
                        "ERR 003 ROOM_NOT_FOUND NID:6548\n");
                }
                else if (!sender_in_room)
                {
                    snprintf(
                        response,
                        sizeof(response),
                        "ERR 006 NOT_IN_ROOM NID:6548\n");
                }
                else
                {
                    snprintf(
                        response,
                        sizeof(response),
                        "OK SENT NID:6548\n");
                }


                              send(client_fd,
                     response,
                     strlen(response),
                     0);
            }


            /* =========================================
             * INVALID / UNKNOWN COMMAND
             * ========================================= */
            if (!known_command)
            {
                char response[1024];

                snprintf(response,
                         sizeof(response),
                         "ERR 005 INVALID_COMMAND NID:6548\n");

                send(client_fd,
                     response,
                     strlen(response),
                     0);
            }
        }
    }


    close(client_fd);


    return NULL;
}


/* =========================================
 * MAIN SERVER
 * ========================================= */
int main()
{
    int server_fd;
    int client_fd;
    int i;

    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;

    socklen_t client_len =
        sizeof(client_addr);


    /* Initialize clients */
    for (i = 0;
         i < MAX_CLIENTS;
         i++)
    {
        clients[i].socket_fd = 0;
        clients[i].username[0] = '\0';
        clients[i].registered = 0;
    }


    /* Initialize rooms */
    for (i = 0;
         i < MAX_ROOMS;
         i++)
    {
        rooms[i].name[0] = '\0';
        rooms[i].member_count = 0;
    }


    /* Create TCP socket */
    server_fd =
        socket(AF_INET,
               SOCK_STREAM,
               0);


    if (server_fd < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }


    /* Configure server address */
    memset(&server_addr,
           0,
           sizeof(server_addr));


    server_addr.sin_family =
        AF_INET;

    server_addr.sin_addr.s_addr =
        INADDR_ANY;

    server_addr.sin_port =
        htons(PORT);


    /* Bind */
    if (bind(server_fd,
             (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0)
    {
        perror("bind");

        close(server_fd);

        exit(EXIT_FAILURE);
    }


    /* Listen */
    if (listen(server_fd,
               BACKLOG) < 0)
    {
        perror("listen");

        close(server_fd);

        exit(EXIT_FAILURE);
    }


    printf("NetMessenger Server Started\n");

    printf("Listening on port %d...\n",
           PORT);


    /* Accept clients */
    while (1)
    {
        client_len =
            sizeof(client_addr);


        client_fd =
            accept(server_fd,
                   (struct sockaddr *)&client_addr,
                   &client_len);


        if (client_fd < 0)
        {
            perror("accept");
            continue;
        }


        printf("Client connected.\n");
	log_event("Client connected");

        int client_index =
            add_client(client_fd);


        if (client_index < 0)
        {
            printf(
                "Server full. Connection rejected.\n");

            close(client_fd);

            continue;
        }


        printf(
            "Client assigned to slot %d.\n",
            client_index);


        ThreadData *data =
            malloc(sizeof(ThreadData));


        if (data == NULL)
        {
            perror("malloc");

            close(client_fd);

            cleanup_client(client_index);

            continue;
        }


        data->client_index =
            client_index;


        pthread_t thread_id;


        if (pthread_create(
                &thread_id,
                NULL,
                handle_client,
                data) != 0)
        {
            perror("pthread_create");

            close(client_fd);

            free(data);

            cleanup_client(client_index);

            continue;
        }


        pthread_detach(thread_id);
    }


    close(server_fd);

    return 0;
}
