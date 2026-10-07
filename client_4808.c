#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <pthread.h>

#define PORT 10808

pthread_mutex_t screen_mutex = PTHREAD_MUTEX_INITIALIZER;

void *receive_messages(void *arg)
{
    int client_fd = *(int *)arg;

    char input_buffer[8192];
    int input_length = 0;

    while (1)
    {
        int bytes_received =
            recv(client_fd,
                 input_buffer + input_length,
                 sizeof(input_buffer) - input_length,
                 0);

        if (bytes_received <= 0)
        {
            break;
        }

        input_length += bytes_received;

        while (1)
        {
            /* Look for a complete line */
            char *newline =
                memchr(input_buffer, '\n', input_length);

            if (newline == NULL)
            {
                break;
            }

            int header_length =
                (int)(newline - input_buffer) + 1;

            char header[1024];

            if (header_length >= sizeof(header))
            {
                printf("Server: Invalid header\n");

                memmove(input_buffer,
                        input_buffer + header_length,
                        input_length - header_length);

                input_length -= header_length;
                continue;
            }

            memcpy(header,
                   input_buffer,
                   header_length);

            header[header_length] = '\0';

            /*
             * Check whether this is a FILE header.
             * Format:
             * FILE <sender> <filename> <bytes>\n
             */
            if (strncmp(header, "FILE ", 5) == 0)
            {
                char sender[100];
                char filename[256];
                long file_size;

                if (sscanf(header + 5,
                           "%99s %255s %ld",
                           sender,
                           filename,
                           &file_size) != 3 ||
                    file_size < 0)
                {
                    printf("Server: Invalid FILE header\n");

                    memmove(input_buffer,
                            input_buffer + header_length,
                            input_length - header_length);

                    input_length -= header_length;
                    continue;
                }

                /*
                 * We need the complete file before saving it.
                 */
                if (input_length - header_length < file_size)
                {
                    break;
                }

                /* Create receiver directory */
                system("mkdir -p received_files");

                char output_path[512];

                snprintf(output_path,
                         sizeof(output_path),
                         "received_files/%s",
                         filename);

                FILE *file =
                    fopen(output_path, "wb");

                if (file == NULL)
                {
                    perror("fopen");

                    /*
                     * Remove the header and file from
                     * the input buffer even if saving failed.
                     */
                    memmove(input_buffer,
                            input_buffer + header_length + file_size,
                            input_length -
                            header_length -
                            file_size);

                    input_length -=
                        header_length + file_size;

                    continue;
                }

                fwrite(input_buffer + header_length,
                       1,
                       file_size,
                       file);

                fclose(file);

                pthread_mutex_lock(&screen_mutex);

                printf("\r\033[K");
                printf("Server: FILE %s %s %ld\n",
                       sender,
                       filename,
                       file_size);

                printf("File saved: %s/%s (%ld bytes)\n",
                       "received_files",
                       filename,
                       file_size);

                printf("You: ");
                fflush(stdout);

                pthread_mutex_unlock(&screen_mutex);

                /*
                 * Remove processed header + file bytes
                 * from the input buffer.
                 */
                memmove(input_buffer,
                        input_buffer + header_length + file_size,
                        input_length -
                        header_length -
                        file_size);

                input_length -=
                    header_length + file_size;

                continue;
            }

            /*
             * Normal server message.
             * Remove the complete line from the buffer.
             */
            pthread_mutex_lock(&screen_mutex);

            printf("\r\033[K");
            printf("Server: %s", header);

            printf("You: ");
            fflush(stdout);

            pthread_mutex_unlock(&screen_mutex);

            memmove(input_buffer,
                    input_buffer + header_length,
                    input_length - header_length);

            input_length -= header_length;
        }
    }

    return NULL;
}


int main()
{
    int client_fd;
    struct sockaddr_in server_addr;

    client_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (client_fd < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(client_fd,
                (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0)
    {
        perror("connect");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    printf("Connected to NetMessenger server.\n");

    /*
     * Receive welcome message before starting
     * the receiver thread.
     */
    char welcome[1024];
    int welcome_bytes;

    welcome_bytes = recv(client_fd,
                         welcome,
                         sizeof(welcome) - 1,
                         0);

    if (welcome_bytes <= 0)
    {
        perror("recv welcome");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    welcome[welcome_bytes] = '\0';

    printf("Server: %s", welcome);

    pthread_t receiver_thread;

    if (pthread_create(&receiver_thread,
                       NULL,
                       receive_messages,
                       &client_fd) != 0)
    {
        perror("pthread_create");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    pthread_detach(receiver_thread);

    char buffer[1024];

    while (1)
    {
        pthread_mutex_lock(&screen_mutex);

        printf("You: ");
        fflush(stdout);

        pthread_mutex_unlock(&screen_mutex);

        if (fgets(buffer, sizeof(buffer), stdin) == NULL)
        {
            break;
        }

        /* Check for SENDFILE command */
        if (strncmp(buffer, "SENDFILE ", 9) == 0)
        {
            char target[100];
            char filename[256];
            long file_size;

            if (sscanf(buffer + 9,
                       "%99s %255s %ld",
                       target,
                       filename,
                       &file_size) != 3)
            {
                printf("Invalid SENDFILE format.\n");
                continue;
            }

		FILE *file = fopen(filename, "rb");

if (file == NULL)
{
    perror("fopen");
    continue;
}

/* Check maximum allowed file size */
if (file_size > 5 * 1024 * 1024)
{
    printf("File too large. Maximum allowed size is 5 MB.\n");
    fclose(file);
    continue;
}

/* Send SENDFILE header */
if (send(client_fd,
         buffer,
         strlen(buffer),
         0) < 0)
{


                perror("send");
                fclose(file);
                break;
            }

            /* Send exactly file_size bytes */
            char file_buffer[1024];
            long total_sent = 0;

            while (total_sent < file_size)
            {
                long remaining = file_size - total_sent;

                int to_read =
                    remaining < sizeof(file_buffer)
                    ? (int)remaining
                    : sizeof(file_buffer);

                int bytes_read =
                    fread(file_buffer,
                          1,
                          to_read,
                          file);

                if (bytes_read <= 0)
                {
                    break;
                }

                int sent_total = 0;

                while (sent_total < bytes_read)
                {
                    int sent =
                        send(client_fd,
                             file_buffer + sent_total,
                             bytes_read - sent_total,
                             0);

                    if (sent <= 0)
                    {
                        perror("send file");
                        fclose(file);
                        close(client_fd);
                        return 0;
                    }

                    sent_total += sent;
                }

                total_sent += bytes_read;
            }

            fclose(file);

            printf("File sent: %s (%ld bytes)\n",
                   filename,
                   total_sent);

            continue;
        }

        /* Normal command */
        if (send(client_fd,
                 buffer,
                 strlen(buffer),
                 0) < 0)
        {
            perror("send");
            break;
        }


    }

    close(client_fd);

    return 0;
}
