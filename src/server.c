#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>

#define PORT 8080
#define BACKLOG 5 // Max pending connections
#define STRING_LENGTH 11
#define UPLOAD_DIR "uploads/"

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

void append_png(char *str, char ext[32])
{
    strcat(str, ".");
    strcat(str, ext);
}

void *handle_client(void *arg)
{
    int client_fd = *(int *)arg;
    char filename[STRING_LENGTH + 1 + 4];
    char filepath[STRING_LENGTH + 1 + 4 + sizeof(UPLOAD_DIR)];

    generate_random_string(filename, STRING_LENGTH);

    char file_extension[32];
    ssize_t file_extension_len = recv(client_fd, file_extension, sizeof(file_extension) - 1, 0);
    if (file_extension_len <= 0) {
        perror("Failed to receive file extension");
        close(client_fd);
        pthread_exit(NULL);
    }
    file_extension[strlen(file_extension) + 1] = '\0';
    printf("Received '.%s' file.\n", file_extension);

    append_png(filename, file_extension);

    snprintf(filepath, sizeof(filepath), "%s%s", UPLOAD_DIR, filename);

    struct stat st;
    if (stat(UPLOAD_DIR, &st) == -1)
        if (mkdir(UPLOAD_DIR, 0700))
        {
            perror("Failed to create uploads directory");
            close(client_fd);
            pthread_exit(NULL);
        }

    FILE *fp = fopen(filepath, "wb");
    if (fp == NULL)
    {
        printf("Failed to create file '%s'", filename);
        close(client_fd);
        pthread_exit(NULL);
    }

    const char *ack = "Ready to receive file data";
    ssize_t sent_bytes = send(client_fd, ack, strlen(ack), 0);
    if (sent_bytes < 0) {
        perror("Response write failed");
        fclose(fp);
        close(client_fd);
        pthread_exit(NULL);
    }

    char buffer[1024];
    ssize_t bytes_read;

    while ((bytes_read = read(client_fd, buffer, sizeof(buffer))) > 0)
    {
        fwrite(buffer, 1, bytes_read, fp);
    }

    if (bytes_read < 0)
    {
        perror("Read error");
        fclose(fp);
        close(client_fd);
        pthread_exit(NULL);
    }

    fflush(fp);
    fclose(fp);
    close(client_fd);
    pthread_exit(NULL);
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

        // Return server response if successful
        const char *response = "File uploaded successfully";
        ssize_t sent_bytes = send(client_fd, response, strlen(response), 0);
        if (sent_bytes < 0)
            perror("Response write failed");
        else
            printf("Response sent to client: %s\n", response);

        puts("File uploaded successfully!\n");
        pthread_detach(tid);
    }

    close(server_fd);

    return 0;
}
