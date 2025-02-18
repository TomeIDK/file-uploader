CC = gcc                    # The compiler to use, here it is GCC
CFLAGS = -Wall -g            # Compiler flags, '-Wall' enables warnings, '-g' includes debugging info
SERVER_SRC = src/server.c src/file_ops.c src/utils.c  # List of source files for the server
CLIENT_SRC = src/client.c src/file_ops.c src/utils.c  # List of source files for the client
SERVER_OBJ = $(SERVER_SRC:.c=.o)  # Compiled object files for the server
CLIENT_OBJ = $(CLIENT_SRC:.c=.o)  # Compiled object files for the client
OUTPUT_SERVER = file_server      # Output executable name for the server
OUTPUT_CLIENT = file_client      # Output executable name for the client

# The 'all' target is the default when you run 'make'
all: $(OUTPUT_SERVER) $(OUTPUT_CLIENT)

# Rule for creating the server executable
$(OUTPUT_SERVER): $(SERVER_OBJ)
	$(CC) $(SERVER_OBJ) -o $(OUTPUT_SERVER)

# Rule for creating the client executable
$(OUTPUT_CLIENT): $(CLIENT_OBJ)
	$(CC) $(CLIENT_OBJ) -o $(OUTPUT_CLIENT)

# 'clean' target to remove all compiled files
clean:
	rm -f $(SERVER_OBJ) $(CLIENT_OBJ) $(OUTPUT_SERVER) $(OUTPUT_CLIENT)
