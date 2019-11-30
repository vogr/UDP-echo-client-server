#ifndef SERVER_H
#define SERVER_H

int parse_port(char*, unsigned short*);
char *text_of(struct sockaddr *address);

#endif //SERVER_H
