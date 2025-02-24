#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080

int main(int argc, char const *argv[])
{
    if (argc < 2)
    {
        printf("Usage: %s <image_file> [directory]\n", argv[0]);
        exit(1);
    }

    int sockfd;
    struct sockaddr_in server_addr;
    FILE *fp;
    char buffer[1024];
    ssize_t bytes_read;
    const char *image_filename = argv[1];
    const char *target_dir = NULL;
    if (argc > 2)
    {
        target_dir = argv[2];
    }
    char *filename_copy = strdup(image_filename);
    if (filename_copy == NULL)
    {
        perror("Memory allocation failed");
        exit(1);
    }

    char *file_extension = strrchr(filename_copy, '.');
    if (file_extension != NULL)
    {
        file_extension++;
    }
    else
    {
        file_extension = "";
    }

    char extension_to_send[32];
    snprintf(extension_to_send, sizeof(extension_to_send), "%s", file_extension);

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("Failed to create socket\n");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Connect to server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Failed to connect to server\n");
        close(sockfd);
        exit(1);
    }

    send(sockfd, extension_to_send, strlen(extension_to_send), 0);

    char ack[1024];
    int ack_len = recv(sockfd, ack, sizeof(ack), 0);
    if (ack_len <= 0)
    {
        printf("Failed to receive acknowledgment from server.\n");
        close(sockfd);
        exit(1);
    }

    printf("Server acknowledged 1: %s\n", ack);

    // if (target_dir != NULL)
    // {
    send(sockfd, target_dir, strlen(target_dir), 0);
    char ack_target_dir[1024];
    int ack_target_dir_len = recv(sockfd, ack_target_dir, sizeof(ack_target_dir), 0);
    if (ack_target_dir_len <= 0)
    {
        printf("Failed to receive acknowledgment from server.\n");
        close(sockfd);
        exit(1);
    }
    printf("Server acknowledged 2: %s\n", ack_target_dir);
    // }

    // Open file to upload
    fp = fopen(image_filename, "rb");
    if (fp == NULL)
    {
        printf("Failed to open file '%s'\n", image_filename);
        close(sockfd);
        exit(1);
    }

    // Send file data in chunks
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0)
    {
        ssize_t sent_bytes = send(sockfd, buffer, bytes_read, 0);
        printf("sent_bytes: %zd\n", sent_bytes);
        if (sent_bytes == -1)
        {
            perror("Send failed");
            fclose(fp);
            close(sockfd);
            exit(1);
        }

        if (sent_bytes < bytes_read)
        {
            printf("Not all bytes were sent, %zd of %zd\n", sent_bytes, bytes_read);
        }
    }
    fclose(fp);
    printf("File sent successfully!\n");

    close(sockfd);
    puts("Closed socket connection");

    return 0;
}
