#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

#define PORT 8080
#define BACKLOG 5 // Max pending connections
#define STRING_LENGTH 32
#define FILE_BUFFER 1024

void generate_random_string(char *str, size_t length)
{
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    size_t charset_size = sizeof(charset) - 1;

    for (size_t i = 0; i < length; i++)
    {
        int random_index = rand() % charset_size;
        str[i] = charset[random_index];
    }

    str[length] = '\0';
}

void append_ext(char *str, char *ext)
{
    strcat(str, ".");
    strcat(str, ext);
}

void create_directory(const char *path)
{
    char temp[256];
    char *p = NULL;
    size_t len;

    snprintf(temp, sizeof(temp), "%s", path);
    len = strlen(temp);
    if (temp[len - 1] == '/')
        temp[len - 1] = '\0';

    for (p = temp + 1; *p; p++)
    {
        if (*p == '/')
        {
            *p = '\0';
            if (mkdir(temp, 0700) && errno != EEXIST)
            {
                perror("mkdir failed");
            }
            *p = '/';
        }
    }
    if (mkdir(temp, 0700) && errno != EEXIST)
    {
        perror("mkdir failed");
    }
}

// Read metadata from socket_fd
void read_data(int socket_fd, char *metadata, size_t metadata_size)
{

    ssize_t metadata_len = recv(socket_fd, metadata, metadata_size - 1, 0);
    if (metadata_len <= 0)
    {
        if (metadata_len == 0)
            printf("Client closed the connection. \n");
        else
            perror("Failed to receive data");
        close(socket_fd);
        pthread_exit(NULL);
    }
    metadata[metadata_len] = '\0';
    printf("Received data: '%s'\n", metadata);
}

// Acknowledge incoming data was received to client
ssize_t acknowledge(int socket_fd, char *msg)
{
    ssize_t bytes_sent = send(socket_fd, (const void *)msg, strlen(msg) + 1, 0);
    if (bytes_sent < 0)
    {
        perror("Failed to send acknowledgment");
        return -1;
    }
    return bytes_sent;
}

// Read incoming file data and write to *fp filepath
ssize_t transfer_file(int socket_fd, int buffer_len, FILE *fp)
{
    char *buffer = malloc(buffer_len);
    if (!buffer)
    {
        perror("Memory allocation failed");
        return -1;
    }
    ssize_t bytes_read;
    ssize_t total_bytes = 0;

    while ((bytes_read = read(socket_fd, buffer, buffer_len)) > 0)
    {
        if (fwrite(buffer, 1, bytes_read, fp) != bytes_read)
        {
            perror("Write error");
            free(buffer);
            return -1;
        }
        total_bytes += bytes_read;
    }
    if (bytes_read < 0)
    {
        perror("Read error");
        free(buffer);
        fclose(fp);
        pthread_exit(NULL);
    }

    free(buffer);
    return total_bytes;
}

void *handle_client(void *arg)
{
    int client_fd = *(int *)arg;
    char filename[STRING_LENGTH + 8];
    char upload_dir[256] = "./uploads/";

    generate_random_string(filename, STRING_LENGTH);

    // Get file extension => Transform so it's based on file's magic number
    char file_extension[8];
    read_data(client_fd, file_extension, sizeof(file_extension));

    // Acknowledge file extension was received
    char ack_file_extension[] = "File extension received successfully\n";
    acknowledge(client_fd, ack_file_extension);

    // Get target directory
    char target_dir[32];
    read_data(client_fd, target_dir, sizeof(target_dir));
    target_dir[strlen(target_dir)] = '/';

    // Acknowledge target directory was received
    char ack_target_dir[] = "Target directory received successfully\n";
    acknowledge(client_fd, ack_target_dir);

    // Append extension to filename
    append_ext(filename, file_extension);

    // Set up upload path
    char full_upload_dir[320];
    mkdir(upload_dir, 0700);
    snprintf(full_upload_dir, sizeof(full_upload_dir), "%s%s", upload_dir, target_dir);

    create_directory(full_upload_dir);

    size_t filepath_size = strlen(full_upload_dir) + strlen(filename) + 1;
    char *filepath = malloc(filepath_size);
    if (filepath == NULL)
    {
        perror("Filepath memory allocation failed");
        pthread_exit(NULL);
    }
    snprintf(filepath, filepath_size, "%s%s", full_upload_dir, filename);
    printf("Target path: %s\n", filepath);

    // Set up file pointer
    FILE *fp = fopen((const char *)filepath, "wb");
    if (fp == NULL)
    {
        perror("Failed to create file");
        printf("Error opening: '%s'\n", filepath);
        free(filepath);
        close(client_fd);
        pthread_exit(NULL);
    }

    // Start reading incoming file
    ssize_t total_bytes = transfer_file(client_fd, FILE_BUFFER, fp);
    if (total_bytes == -1)
    {
        perror("Error writing file");
        fclose(fp);
    }
    printf("total_bytes: %zd\n", total_bytes);

    fflush(fp);
    fclose(fp);
    free(filepath);

    // Acknowledge file was uploaded
    char ack_file_upload[] = "File uploaded successfully\n";
    acknowledge(client_fd, ack_file_upload);

    close(client_fd);
}

int main()
{
    srand((unsigned)time(NULL));
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;

    // Create a socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0)
    {
        perror("Socket creation failed");
        exit(1);
    }

    // Bind socket to address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Binding failed");
        exit(1);
    }

    if (listen(server_fd, BACKLOG) < 0)
    {
        perror("Listen failed");
        exit(1);
    }
    printf("Server started!\n");

    // Handle multiple clients
    pthread_t tid;
    while (1)
    {
        // Accept a client connection
        socklen_t addr_size = sizeof(client_addr);
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_size);
        if (client_fd < 0)
        {
            perror("Client connection failed");
            continue;
        }
        printf("Client connected!\n");

        // Create a new thread for the client
        if (pthread_create(&tid, NULL, handle_client, (void *)&client_fd) != 0)
        {
            perror("Thread creation failed");
            close(client_fd);
        }
        pthread_detach(tid);
    }

    close(server_fd);

    return 0;
}
