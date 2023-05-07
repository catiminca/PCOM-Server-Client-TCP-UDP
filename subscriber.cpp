#include <arpa/inet.h>
#include <math.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#include "helpers.h"

struct pollfd poll_fds[MAX_CONNECTIONS];
int nr_polls = 0;

int stdin_case(int fd) {
    char buff[MAX_CONTENT_LEN];
    memset(buff, 0, MAX_CONTENT_LEN);
    fgets(buff, MAX_CONTENT_LEN, stdin);
    if (strncmp(buff, "exit", 4) == 0) {
        return 0;
    }
    // este subscribe sau unsubscribe
    packet_type type;
    int len;
    char *aux = strtok(buff, " ");
    subscription subcriber;
    if (strcmp(aux, "subscribe") == 0) {
        type = SUBSCRIBE;
        char topic_name[51];

        aux = strtok(NULL, " ");
        strcpy(topic_name, aux);
        len = strlen(topic_name);

        aux = strtok(NULL, " ");
        int sf = *aux - '0';

        if (sf == 1)
            type = SUBSCRIBE_SF;

        send_all(fd, &type, sizeof(packet_type));
        send_all(fd, &len, sizeof(len));

        send_all(fd, topic_name, len);

        std::cout << "Subscribed to topic.\n";
    } else if (strcmp(aux, "unsubscribe") == 0) {
        type = UNSUBSCRIBE;
        send_all(fd, &type, sizeof(packet_type));
        // len = strlen(buff);
        send_all(fd, &len, sizeof(int));

        aux = strtok(NULL, " ");
        strcpy(subcriber.topic, aux);
        int rc = send_all(fd, &subcriber, len);
        DIE(rc < 0, "send");
        std::cout << "Unsubscribed from topic.\n";
    }
    return 1;
}

int udp_handler_msg(int sockfd) {
    udp message;
    memset(&message, 0, sizeof(udp));

    packet_type type;
    recv_all(sockfd, &type, sizeof(packet_type));
    int len;
    recv_all(sockfd, &len, sizeof(int));
    if (type == MESSAGE) {
        size_t bytes_received = 0;
        bytes_received = recv_all(sockfd, &message, len);

        if (bytes_received == 0) {
            // close(sockfd);
            return 0;
        }

        printf("%s - \n", message.topic);

        // in fct de fiecare tip la data type
        conver_data(message.data_type, message.content);
        return 1;
    }
    return 0;
}

void exit_err() {
    // inchidere poll
    for (int i = 1; i < nr_polls; i++) {
        close(poll_fds[i].fd);
    }
    exit(1);
}

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    int sockfd = -1;
    if (argc < 4) {
        printf("\n Usage: %s <ip> <port>\n", argv[0]);
        return 1;
    }

    // Parsam port-ul ca un numar
    uint16_t port;
    port = atoi(argv[3]);
    int rc;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockfd < 0, "socket");

    int on = 1;
    setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (void *)&on, sizeof(on));

    // Completăm in serv_addr adresa serverului, familia de adrese si portul
    // pentru conectare
    struct sockaddr_in serv_addr;
    socklen_t socket_len = sizeof(struct sockaddr_in);

    memset(&serv_addr, 0, socket_len);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    // serv_addr.sin_addr.s_addr = INADDR_ANY;
    rc = inet_aton(argv[2], &serv_addr.sin_addr);
    DIE(rc == 0, "inet_aton");
    // Ne conectăm la server
    packet_type type = IDENTIFY;
    rc = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(sockaddr_in));
    DIE(rc < 0, "connect");
    nr_polls = 0;

    send_all(sockfd, &type, sizeof(packet_type));
    int len = strlen(argv[1]) + 1;
    send_all(sockfd, &len, sizeof(int));
    char buffer[MAX_CONTENT_LEN];
    memcpy(buffer, argv[1], len);
    rc = send_all(sockfd, buffer, len);

    DIE(rc < 0, "Nu se poate trimite catre server");

    poll_fds[0].fd = STDIN_FILENO;
    poll_fds[0].events = POLLIN;
    nr_polls++;

    // se creaza socketul serverului

    poll_fds[1].fd = sockfd;
    poll_fds[1].events = POLLIN;
    nr_polls++;

    while (1) {
        // se asteapta pana cand se primesc date pe cel putin unul din file descriptori
        poll(poll_fds, nr_polls, -1);

        for (int i = 0; i < nr_polls; i++) {
            if (poll_fds[i].revents & POLLIN) {
                if (poll_fds[i].fd == sockfd) {
                    if (udp_handler_msg(sockfd) == 0)
                        exit_err();
                } else if (poll_fds[i].fd == STDIN_FILENO) {
                    if (stdin_case(sockfd) == 0) {
                        exit_err();
                    }
                }
            }
        }
    }
    // inchidere poll
    for (int i = 1; i < nr_polls; i++) {
        close(poll_fds[i].fd);
    }
    return 0;
}