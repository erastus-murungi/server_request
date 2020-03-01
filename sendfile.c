#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#define BUFSIZE (1000)
#define CHUNKSIZE (10000)
#define BACKLOG (5)
#define LIFETIME 100

int send_file(int sfd, char *filename);

static int init_server(char *portno);

static inline void error(char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
//    if (argc < 2) {
//        fprintf(stderr, "Port number to open socket\n");
//        exit(EXIT_FAILURE);
//    }
//    init_server(argv[1]);
    init_server("9000"); //
    return EXIT_SUCCESS;
}


static int init_server(char *portno) {
    /**
     * we are the server, we receive file requests and fulfil them
     * waits for one request at a time and sends only file
     * can be modified to send multiple files. If a file is not in the files folder, then an error is raised
     * and the user is sent a message of -1
     * else a message of 1 for success
     */

    char filename[BUFSIZE];
    int sockfd, newsockfd, status, yes;
    socklen_t size;
    struct addrinfo hints, *my_addr;
    struct sockaddr_storage their_addr;

    if (atoi(portno) < 1024) {
        fprintf(stderr, "port numbers < 1024 are system reserved\n");
        exit(EXIT_FAILURE);
    }

    // hints
    size = sizeof(struct sockaddr_storage);
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    // the first argument can be localhost or null
    if ((status = (getaddrinfo("localhost", portno, &hints, &my_addr))) < 0)
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    // open a socket with listens on my IP at port 3000;
    // int socket(int domain, int type, int protocol);
    if ((sockfd = socket(my_addr->ai_family, my_addr->ai_socktype, my_addr->ai_protocol)) < 0) {
        error("socket opening");
    }
    fprintf(stdout, "socket opened successfully\n");
    // initialize the server socket struct
    // int bind(int sockfd, struct sockaddr *my_addr, int addrlen);
    // some little bit of a socket might still be hogging the port
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
        error("setsockopt");
    }

    if (bind(sockfd, my_addr->ai_addr, my_addr->ai_addrlen) < 0) {
        error("binding socket");
    }

    fprintf(stdout, "socket bound successfully...\n");
    listen(sockfd, BACKLOG);
    fprintf(stdout, "waiting for incoming connections...\n");

    struct timeval tp;
    gettimeofday(&tp, NULL);
    long start = tp.tv_sec;
    long now;

    do {
        // select
        if ((newsockfd = accept(sockfd, (struct sockaddr *) &their_addr, &size)) < 0) {
            error("accept");
        }
        fprintf(stdout, "connection accepted\n");
        fflush(stdout); // clear output
        // first read the file name
        memset(filename, 0, BUFSIZE);
        do {
            status = recv(newsockfd, filename, sizeof(filename), 0);
        } while (status < 0);

        printf("filename: %s\n", filename);

        char *path = malloc(BUFSIZE);
        strcpy(path, "/Users/erastusmurungi/CLionProjects/intro_socket/files/");

        strcat(path, (char *) filename);

        send_file(newsockfd, (char *) path);
        fflush(stdout);
        fprintf(stdout, "waiting for incoming connections...\n");

        gettimeofday(&tp, NULL);
        now = tp.tv_sec;

    } while ((now - start) < LIFETIME);
    printf("server closing after %i seconds.. \n", (int) (now - start));

    close(sockfd);
    fflush(stdout);
    free(my_addr);
    return EXIT_SUCCESS;
}

int send_file(int sfd, char *filename) {
    FILE *file;
    long filesize;
    double progress;
    u_long readsize, sent, total, i;
    char send_buffer[CHUNKSIZE], ack[3];

    file = fopen(filename, "rb");
    total = 0;

    // try to read the file into the file pointer
    if (file == NULL) {
        error("");
    }
    // get file size
    fseek(file, 0L, SEEK_END);
    filesize = ftell(file);
    fseek(file, 0L, SEEK_SET);

    fprintf(stdout, "sending picture size: %li \n", filesize);
    sent = send(sfd, &filesize, sizeof filesize, 0);
    if (sent < 0) {
        error("send");
    }
    do {
        i = recv(sfd, &ack, 3, 0);
    } while (i < 0 && (strncmp(ack, "OK", 3) != 0));

    printf("acknowledgment received..\n");

    memset(send_buffer, 0, sizeof(send_buffer));

    fprintf(stdout, "sending picture as a byte array... \n");
    while (!feof(file)) {
        readsize = fread(send_buffer, 1, sizeof(send_buffer) - 1, file);
        send_buffer[readsize] = '\0'; //
        do {
            sent = send(sfd, &send_buffer, readsize, 0);
        } while (sent < 0);

        total += sent;
        progress = (double) total / (double) filesize;
        fprintf(stdout, "(%lu / %ld) bytes sent %lf %% \n", total, filesize, progress * 100);
        memset(send_buffer, 0, CHUNKSIZE); // clear buffer
    }
    return EXIT_SUCCESS;
}