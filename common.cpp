#include "common.h"

#include <bits/stdc++.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <string>

#include "helpers.h"

int recv_all(int sockfd, void *buffer, size_t len) {
    size_t bytes_received = 0;
    size_t bytes_remaining = len;
    char *buff = (char *)buffer;

    while (bytes_remaining) {
        int rc = recv(sockfd, buff + bytes_received, bytes_remaining, 0);
        DIE(rc == -1, "recv() failed!\n");
        if (rc == 0)
            return bytes_received;

        bytes_received += rc;
        bytes_remaining -= rc;
    }

    return bytes_received;
}

/*
        TODO 1.2: Rescrieți funcția de mai jos astfel încât ea să facă trimiterea
        a exact len octeți din buffer.
*/
int send_all(int sockfd, void *buffer, size_t len) {
    size_t bytes_sent = 0;
    size_t bytes_remaining = len;
    char *buff = (char *)buffer;

    while (bytes_remaining) {
        int rc = send(sockfd, buff + bytes_sent, bytes_remaining, 0);
        DIE(rc == -1, "send() failed!\n");
        if (bytes_sent == 0)
            return bytes_sent;

        bytes_sent += rc;
        bytes_remaining -= rc;
    }

    return bytes_sent;
}

uint8_t devide_by_10(uint8_t nr) {
    uint8_t aux = nr;
    while (nr > 0) {
        nr = nr / 10;
        aux--;
    }
    return nr;
}

void conver_data(char data_type, char content[MAX_UDP_PACKET_LEN]) {
    if (data_type == 0) {
        std::cout << "INT - ";
        u_int32_t number;
        memcpy(&number, content + 1, sizeof(u_int32_t));
        number = ntohl(number);
        if (content[0] == 1) {
            std::cout << "-" << number << "\n";
        } else {
            std::cout << number << "\n";
        }
    } else if (data_type == 1) {
        std::cout << "SHORT_REAL - ";
        u_int16_t number;
        memcpy(&number, content, sizeof(u_int32_t));
        number = ntohs(number);
        std::cout << std::setprecision(2) << (float)number / 100 << "\n";
    } else if (data_type == 2) {
        std::cout << "FLOAT - ";
        uint32_t number;
        uint8_t rest;

        memcpy(&number, content + 1, sizeof(uint32_t));
        memcpy(&rest, content + 1 + sizeof(uint32_t), sizeof(uint8_t));

        number = ntohl(number);
        uint8_t result = devide_by_10(rest);
        if (content[0] == 1) {
            std::cout << "-" << std::setprecision(number) << result << "\n";
        } else {
            std::cout << std::setprecision(number) << result << "\n";
        }
    } else if (data_type == 3) {
        std::cout << "STRING - " << content << "\n";
    }
}