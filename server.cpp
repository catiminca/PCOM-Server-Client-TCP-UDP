#include <arpa/inet.h>
#include <ctype.h>
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

#include <iostream>

#include "common.h"
#include "helpers.h"

udp udp_msg;

int topics_nr = 0;
struct sockaddr_in udp_addr;
struct sockaddr_in serv_addr;
std::vector<pollfd> poll_fds;
std::unordered_map<std::string, std::vector<packet>> clients_messages;
std::unordered_map<int, sockaddr_in> fd_addr;
std::unordered_map<int, std::string> clients_fd;
std::unordered_map<std::string, int> fd_clients;
std::unordered_map<std::string, std::unordered_map<std::string, bool>> all_topics;

int stdin_case(char buff[]) {
    memset(buff, 0, MAX_CONTENT_LEN);
    fgets(buff, MAX_CONTENT_LEN, stdin);
    if (strncmp(buff, "exit", 4) == 0) {
        return 0;
    }
    return 1;
}

packet create_packet(udp msg, int len) {
    packet pac;
    pac.buff = (char *)malloc(len);
    memcpy(pac.buff, &udp_msg, len);
    pac.len = len;
    return pac;
}

/**
 *  primeste mesaj de la un client UDP si il trimite
 *  mai departe acelor clienti TCP care sunt abonati la topic-ul mesajului
 */
void udp_case(int udp_socket) {
    memset(&udp_msg, 0, sizeof(udp_msg));
    int rc;
    int len;
    rc = recvfrom(udp_socket, &udp_msg, MAX_UDP_PACKET_LEN, 0, NULL, NULL);
    DIE(rc < 0, "recv udp msg");
    len = rc;

    for (auto subscriber : all_topics[udp_msg.topic]) {
        // on
        if (fd_clients[subscriber.first] != -1) {
            packet_type type = MESSAGE;
            send_all(fd_clients[subscriber.first], &type, sizeof(packet_type));
            send_all(fd_clients[subscriber.first], &len, sizeof(int));
            rc = send_all(fd_clients[subscriber.first], &udp_msg, len);
            DIE(rc < 0, "send when client on-udp");
        }
        // se pastreaza sa fie trimis mai tarziu daca e abonat
        else if (subscriber.second) {
            packet packet_curr = create_packet(udp_msg, len);
            clients_messages[subscriber.first].push_back(packet_curr);
        }
    }
}

/**
 * accepta sau refuza conexiuni cu clientii TCP
 */
void tcp_case_listen(int tcp_socket) {
    int rc;
    char buff[50];
    char id[MAX_ID_LEN];
    struct sockaddr_in cli_addr;
    socklen_t cli_len = sizeof(sockaddr_in);
    memset(&cli_addr, 0, sizeof(sockaddr_in));
    int newsockfd = accept(tcp_socket, (struct sockaddr *)&cli_addr, &cli_len);
    DIE(newsockfd < 0, "accept");
    poll_fds.push_back({pollfd{newsockfd, POLLIN, 0}});
    fd_addr[newsockfd] = cli_addr;
}

int main(int argc, char **argv) {
    // dezactivare buffering la afisare
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    int num_clients = 0;
    socklen_t socket_len = sizeof(struct sockaddr_in);
    char buff[MAX_CONTENT_LEN], id[MAX_ID_LEN];

    if (argc != 2) {
        printf("\n Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    uint16_t port;
    int rc;
    port = atoi(argv[1]);

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(listenfd < 0, "tcp_socket");

    int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    DIE(udp_socket < 0, "udp_socket");

    // Facem adresa socket-ului reutilizabila, ca sa nu primim eroare in caz ca
    // rulam de 2 ori rapid
    int enable = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    // Completăm in serv_addr adresa serverului, familia de adrese si portul
    // pentru conectare
    memset(&serv_addr, 0, socket_len);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    rc = bind(listenfd, (const struct sockaddr *)&serv_addr, sizeof(sockaddr_in));
    DIE(rc < 0, "bind_tcp");

    rc = bind(udp_socket, (const struct sockaddr *)&serv_addr, sizeof(sockaddr_in));
    DIE(rc < 0, "bind_udp");

    rc = listen(listenfd, MAX_CONNECTIONS);
    DIE(rc < 0, "listen_tcp");

    poll_fds.push_back(pollfd{STDIN_FILENO, POLLIN, 0});
    poll_fds.push_back(pollfd{listenfd, POLLIN, 0});
    poll_fds.push_back(pollfd{udp_socket, POLLIN, 0});

    while (1) {
        rc = poll(poll_fds.data(), poll_fds.size(), -1);
        DIE(rc < 0, "poll");
        if (poll_fds[0].revents & POLLIN) {
            // primire date de la stdin
            if (stdin_case(buff) == 0) {
                break;
            }
        }
        for (int i = 0; i < poll_fds.size(); i++) {
            if (poll_fds[i].revents & POLLIN) {
                if (poll_fds[i].fd == udp_socket) {
                    // primire date de la socket UDP
                    udp_case(udp_socket);
                } else if (poll_fds[i].fd == listenfd) {
                    // a venit o cerere de conexiune pe socketul inactiv (cel cu listen),
                    // pe care serverul o accepta
                    tcp_case_listen(listenfd);
                } else {
                    memset(id, 0, MAX_ID_LEN);
                    int fd = poll_fds[i].fd;
                    packet_type type;
                    int rc = recv_all(fd, &type, sizeof(packet_type));

                    if (!rc) {
                        std::string client_name = clients_fd[fd];

                        std::cout << "Client" << client_name << "disconnected.\n";
                        fd_clients[client_name] = -1;
                    }

                    int len;
                    recv_all(fd, &len, sizeof(int));
                    if (type == IDENTIFY) {
                        rc = recv_all(fd, id, len);
                        DIE(rc < 0, "recv ids");

                        if (fd_clients.count(id) && fd_clients[id] != -1) {
                            std::cout << "Client " << id << " already connected.\n";
                            close(fd);
                            poll_fds.erase(poll_fds.begin() + i);
                            continue;
                        }

                        clients_fd[fd] = id;
                        fd_clients[id] = fd;
                        std::cout << "New client " << id << " connected from " << inet_ntoa(fd_addr[fd].sin_addr)
                                  << ":" << ntohs(fd_addr[fd].sin_port) << ".\n";

                        for (auto &packet : clients_messages[id]) {
                            send_all(fd, packet.buff, packet.len);
                        }

                        clients_messages[id].clear();

                    } else if (type == SUBSCRIBE || type == SUBSCRIBE_SF) {
                        char topic_name[MAX_TOPIC_LEN];

                        rc = recv_all(fd, topic_name, len);

                        if (type == SUBSCRIBE_SF)
                            all_topics[topic_name][id] = true;
                        else
                            all_topics[topic_name][id] = false;
                    } else if (type == UNSUBSCRIBE) {
                        char topic_name[MAX_TOPIC_LEN];

                        rc = recv_all(fd, topic_name, len);

                        all_topics[topic_name].erase(id);
                    }
                }
            }
        }
    }

    for (auto &poll_fd : poll_fds)
        close(poll_fd.fd);

    return 0;
}