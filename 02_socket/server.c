#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

int main() 
{
	int server_fd;
	struct sockaddr_in server;

	// Create socket
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		perror("Socket creation failed");
		return 1;
	}

	// Set up server address structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(8080);

	// Bind - assigns addr to socket (server side)
	if (bind(server_fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
		perror("Bind failed");
		return 1;
	}

	// Listen - set socket to listen TCP
	if (listen(server_fd, 3) < 0) {
		perror("Listen failed");
		return 1;
	}

	int new_socket;
	struct sockaddr_in client;
	// Accept TCP connection
	printf("Waiting for connections...\n");
	int addrlen = sizeof(client);
	new_socket = accept(server_fd, (struct sockaddr *)&client, (__socklen_t*)&addrlen);
	if (new_socket < 0) {
		perror("Accept failed");
		return 1;
	}

	char buffer[1024] = {0};
	const char *message = "Hello from server\n";
	// Receive and send data
	recv(new_socket, buffer, sizeof(buffer), 0);
	printf("Client message: %s\n", buffer);
	send(new_socket, message, strlen(message), 0);

	// Close sockets
	close(new_socket);
	close(server_fd);

	return 0;
}