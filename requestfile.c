//
// Created by Erastus Murungi on 2/29/20.
//

#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <assert.h>

#define CHUNKSIZE (10000)

static inline void error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int request_file(char *filename, char *portno);

int receive_file(int sockfd, char *filename);


int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "need both a filename and port number. \n");
        exit(EXIT_FAILURE);
    }
    request_file(argv[1], argv[2]);
    return EXIT_SUCCESS;
}
//int main(void) {
////    request_file("slides.ppt", "9000");
////    return EXIT_SUCCESS;
////}


int request_file(char *filename, char *portno) {
    /**
     * request a file
     */


    if (atoi(portno) < 1024) {
        fprintf(stderr, "port numbers < 1024 are system reserved\n");
        exit(EXIT_FAILURE);
    }

    int server_fd, sent, status;
    struct addrinfo hints, *server;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // the server could be at a different IP

    if ((status = getaddrinfo("127.0.0.1", portno, &hints, &server)) < 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    }
    if ((server_fd = socket(server->ai_family, server->ai_socktype, server->ai_protocol)) < 0) {
        error("socket opening");
    }
    fprintf(stdout, "socket opened successfully\n");

    // int connect(int sockfd, struct sockaddr *serv_addr, int addrlen);
    status = connect(server_fd, server->ai_addr, server->ai_addrlen);
    if (status < 0) {
        close(server_fd);
        error("connect");
    }
    fprintf(stdout, "connection successful \n");
    // write the name of the image
    do {
        sent = send(server_fd, filename, strlen(filename), 0);
    } while (sent < 0 || sent != strlen(filename));
    printf("sent filename\n");
    printf("filename: %s\n", filename);

    receive_file(server_fd, filename);
    close(server_fd);
    return EXIT_SUCCESS;
}

int receive_file(int sockfd, char *filename) {
    int rsize, wsize, filesize, total, packetsize, bufferfd, i, j, sent;
    double progress;
    char buffer[CHUNKSIZE], ack[3] = "OK";
    FILE *file;
    fd_set fds;

    do {
        sent = read(sockfd, &filesize, sizeof(int));
    } while (sent < 0);

    fprintf(stdout, "file size received: %d\n", filesize);
    do{
        i = send(sockfd, &ack, 3, 0);
    } while (i < 0);
    printf("acknowledgement sent... \n");

    file = fopen(filename, "wb");
    if (file == NULL) {
        error("");
    }

    total = 0;
    struct timeval timeout = {20, 10};
    printf("receiving file... \n");
    // crux of the program
    i = 0;
    while (total < filesize) {
        FD_ZERO(&fds);
        FD_SET(sockfd, &fds);

        bufferfd = select(FD_SETSIZE, &fds, NULL, NULL, &timeout);
        if (bufferfd < 0)
            fprintf(stdout, "bad file descriptor\n");
        if (bufferfd == 0) {
            fprintf(stdout, "buffer read timeout expired\n");
        }
        memset(buffer, 0, sizeof(buffer) - 1);
        if (bufferfd > 0) {
            do {
                rsize = recv(sockfd, buffer, CHUNKSIZE - 1, 0);
            } while (rsize < 0);
             // remove th extra bytes
            if (i == 0 && buffer[0] == '\0'){
                j = 4; // where to start
                rsize -= j;
                while (i < rsize) buffer[i++] = buffer[j++];
            }
            wsize = fwrite(buffer, 1, rsize, file); // the sizeof char = 1
            if (rsize != wsize) {
                fprintf(stderr, "incomplete file write \n");
                exit(EXIT_FAILURE);
            }
            total += wsize;
            progress = (double) total / (double) filesize;
            fprintf(stdout, "(%i / %i) bytes received %lf %%\n", total, filesize, progress * 100);
        }
    }
    fseek(file, 0L, SEEK_END);
    long finalsize = ftell(file);
    fseek(file, 0L, SEEK_SET);
    fprintf(stdout, "file size received final: %ld promised: %d \n", finalsize, filesize);

    // remove 4 bits from the beginning ogf the file
    FILE *filecop = fopen("copy", "wb");
    fclose(filecop);
    fclose(file);
    fprintf(stdout, "receive completed.\n");
    return EXIT_SUCCESS;
}
