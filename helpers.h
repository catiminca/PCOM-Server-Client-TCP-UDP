#ifndef _HELPERS_H
#define _HELPERS_H 

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <bits/stdc++.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>

#define MAX_TOPIC_LEN 51
#define MAX_CONTENT_LEN 1500
#define MAX_UDP_PACKET_LEN 1551
#define MAX_ID_LEN 11
#define MAX_CONNECTIONS 32

enum packet_type { 
  IDENTIFY, SUBSCRIBE, UNSUBSCRIBE, MESSAGE, SUBSCRIBE_SF
};

//mesaj primit de la un client udp
struct __attribute__((__packed__))udp {
    char topic[50];
    uint8_t data_type;
    char content[1500];

};

struct packet {
    char *buff;
    int len;
};

struct subscription {
  char topic[MAX_TOPIC_LEN];
  int sf;
};

struct subscriber_topic {
  std::string id;
  bool sf;
};

#define DIE(assertion, call_description)                                       \
  do {                                                                         \
    if (assertion) {                                                           \
      fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__);                       \
      perror(call_description);                                                \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)

#endif