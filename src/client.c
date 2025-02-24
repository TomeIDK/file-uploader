#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 12

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

    fp = fopen(image_filename, "rb");
    if (fp == NULL)
    {
        printf("Failed to open file '%s'\n", image_filename);
        close(sockfd);
        exit(1);
    }

    // Send magic numbers to determine file type
    unsigned char mn_buffer[BUFFER_SIZE];
    size_t mn_bytes_read = fread(mn_buffer, 1, sizeof(mn_buffer), fp);

    if (mn_bytes_read > 0)
    {
        ssize_t bytes_sent = send(sockfd, mn_buffer, mn_bytes_read, 0);
        if (bytes_sent == -1)
        {
            perror("Failed to send magic numbers");
            fclose(fp);
            exit(1);
        }
        puts("Magic numbers sent successfully!\n");
    }
    else
    {
        puts("Failed to read magic numbers from the file\n");
        fclose(fp);
        exit(1);
    }

    fseek(fp, 0, SEEK_END);

    // fp = fopen(image_filename, "rb");
    // if (fp == NULL)
    // {
    //     printf("Failed to open file '%s'\n", image_filename);
    //     close(sockfd);
    //     exit(1);
    // }

    // fseek(fp, 0, SEEK_END);
    // printf("file size %ld\n", ftell(fp));
    // fseek(fp, 0, SEEK_SET);

    // Send file data in chunks
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0)
    {
        printf("bytes_read: %zd\n", bytes_read);
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
