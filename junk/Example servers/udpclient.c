/* A UDP echo client.
 *
 * Read a string from the console, send it by UDP to localhost:32000,
 * receive the reply and print it.
 *
 * Copyright (c) 2016, Marcel Kyas
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Reykjavik University nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MARCEL
 * KYAS NOR REYKJAVIK UNIVERSITY BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
    int sockfd;
    char *message = NULL;
    size_t msg_size;
    struct sockaddr_in server;

    // Create a UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    // Choose an address to connect to.
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);
    server.sin_port = htons(32000);

    // Get a line of input from the user.
    fprintf(stdout, "Type message: ");
    fflush(stdout);
    size_t msg_len = getline(&message, &msg_size, stdin);

    // Send to server without the newline.
    sendto(sockfd, message, msg_len - 1, 0,
           (struct sockaddr *) &server, sizeof(server));

    // Receive reply. recvfrom changes len.
    socklen_t len = (socklen_t) sizeof(server);
    ssize_t n = recvfrom(sockfd, message, msg_len - 1,
                         0, (struct sockaddr *) &server, &len);

    // NUL-terminate the message, otherwise fprintf may access memory
    // outside of the string.
    message[n] = '\0';
    fprintf(stdout, "Received:\n%s\n", message);
    fflush(stdout);

    close(sockfd); // Once we do not need the socket, dispose of it.
    free(message);
    exit(EXIT_SUCCESS);
}
