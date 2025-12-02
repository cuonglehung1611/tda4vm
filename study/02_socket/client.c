#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

int main()
{
	int sock;
    struct sockaddr_in server;
    char message[] = "Hello from client";
    char buffer[1024] = {0};

	// Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Socket creation failed");
        return 1;
    }

	// Set up server address
    server.sin_family = AF_INET;
    server.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

	// Connect to server (client)
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Connection failed");
        return 1;
    }

	// Send data TCP
    send(sock, message, strlen(message), 0);

	// Receive response TCP
    recv(sock, buffer, sizeof(buffer), 0);
    printf("Server reply: %s\n", buffer);

    // Close socket
    close(sock);

	return 0;
}