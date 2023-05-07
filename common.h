#ifndef _COMMON_H
#define _COMMON_H 

int recv_all(int sockfd, void *buffer, long unsigned int len);
int send_all(int sockfd, void *buffer, long unsigned int len);
void conver_data(char data_type, char *content);
#endif