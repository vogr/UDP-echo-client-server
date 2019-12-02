#include <arpa/inet.h>
#include <errno.h>
#include <limits.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "server.h"

#define BUFFER_SIZE 4096
#define MAX_SOCKS 20

int main(int argc, char *argv[]) {
  int ret = 0;

  if (argc < 2) {
    fprintf(stderr, "Error: requires one argument\n\t./server PORT\n");
    return EXIT_FAILURE;
  }


  unsigned short port = 0;
  ret = parse_port(argv[1], &port);
  if (ret) {
    return EXIT_FAILURE;
  }

  /*
   * Implement a dual stack echo server using the ideas from
   * https://stackoverflow.com/a/46627140
   * i.e. bind to a single IPV6 socket which will also handle
   * IPv4 connections using a mapping mechanism: the kernel
   * presents the IPv4 address as an IPv6 address and lets us
   * manipulate it like any IPv6 address.
   *
   * Note: the alternative way would be to bind() to all addresses
   * returned by getaddrinfo (get only bindable addresses with the
   * AI_PASSIVE flag in the sockaddr hints), and to prevent the mapping
   * mechanism by setting IPV6_V6ONLY on sockets for AF_INET6 addresses.
   * The main loop would then rely on threading or on a polling mechanism.
   * See
   * https://tools.ietf.org/html/rfc4038
   * for guidelines and motivations.
   */

  struct sockaddr_in6 addr = {
    .sin6_family = AF_INET6,
    .sin6_port = htons(port),
    .sin6_addr = IN6ADDR_ANY_INIT,
  };

  int sockfd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
  if (sockfd == -1) {
    perror("socket");
    return EXIT_FAILURE;
  }

  /*
   * Enable the IPv4 to IPv6 mapping mechanism by disabling IPV6_V6ONLY.
   * Note: this flag is already off by default on most Linux ditributions.
   * This section
   * https://tools.ietf.org/html/rfc3493#section-5.3
   * explains the IPV6_V6ONLY option and inspired the following lines.
   */
  int off = 0;
  ret = setsockopt(sockfd, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&off, sizeof(off));
  if (ret != 0) {
    char *address_repr = text_of_addr((struct sockaddr *)&addr);
    fprintf(stderr, "Warning: could not disable IPV6_V6ONLY flag on the socket for the address %s\n", address_repr);
    perror("setsockopt IPV6_V6ONLY");
    free(address_repr);
  }

  ret = bind(sockfd, (struct sockaddr *) &addr, sizeof(addr));
  if (ret != 0) {
    char *address_repr = text_of_addr((struct sockaddr *)&addr);
    fprintf(stderr, "Could not bind address %s on port %s\n", address_repr, argv[1]);
    perror("bind");
    free(address_repr);
    close(sockfd);
    return EXIT_FAILURE;
  }
  
  /*
   * If the port entered on the commandline is 0, the OS
   * will find a suitable free port.
   * To report this port to the user, I use a snippet of
   * code taken here:
   * https://stackoverflow.com/a/4047837
   */
	socklen_t addrlen = sizeof(addr);
	ret = getsockname(sockfd, (struct sockaddr *)&addr, &addrlen);
	if (ret) {
    fprintf(stderr, "Cannot dermine which port is being used.\n");
	  perror("getsockname");
  }
  else {
    fprintf(stderr, "Listening on port %d\n", ntohs(addr.sin6_port));
  }



  struct sockaddr_storage from = {0};
  socklen_t from_len = sizeof(from);
  char buffer[BUFFER_SIZE] = {0};

  while (1) {
    ssize_t recvfrom_ret = recvfrom(
        sockfd, buffer, BUFFER_SIZE, 0,
        (struct sockaddr *)&from, &from_len
      );
    if (recvfrom_ret < 0) {
      // could not read
      continue;
    }
    size_t bytes_read = (size_t)recvfrom_ret;

    char *address_repr = text_of_addr((struct sockaddr *)&from);
    fprintf(stderr, "[server] Received bytes from %s, will send them back. Message received:\n", address_repr);
    free(address_repr);

    for(size_t i = 0; i < bytes_read; i++) {
      if (buffer[i] == '\n')
        fprintf(stderr, "\\n");
      else
        fprintf(stderr, "%c", buffer[i]);
    }
    fprintf(stderr, "\n");


    size_t bytes_to_send = bytes_read;
    size_t offset = 0;
    while (bytes_to_send > 0) {
      ssize_t sendto_ret = sendto(
          sockfd, buffer + offset, bytes_to_send, 0,
          (struct sockaddr *)&from, from_len
        );
      if (sendto_ret == -1) {
        perror("sendto");
      }
      size_t bytes_sent = (size_t)sendto_ret;
      offset += bytes_sent;
      bytes_to_send -= bytes_sent;
    }
  }

  return EXIT_SUCCESS;
}


int parse_port(char *s, unsigned short *port) {
  if (s[0] == '\0') {
    fprintf(stderr, "Error: port must not be empty.\n");
    return EXIT_FAILURE;
  }
  char *end = NULL;
  errno = 0;
  long int port_int = strtol(s, &end, 10);
  if (errno) {
    fprintf(stderr, "Invalid port:\n");
    perror("strtol");
    return 1;
  }
  else if (*end != '\0') {
    fprintf(stderr, "Invalid port: some characters were not recognised as digits.\n");
    return 1;
  }
  else if (port_int < 0 || port_int > USHRT_MAX) {
    fprintf(stderr, "Invalid port: value is outside permitted range.\n");
    return 1;
  }

  if (port != NULL) {
    *port = (unsigned short) port_int;
  }
  return 0;
}

// La fonction utilitaire suivante vient du blog
// https://www.bortzmeyer.org/bindv6only.html
// Elle permet d'obtenir une représentation des addresses
// IPv4 et IPv6.
char *text_of_addr(struct sockaddr *address) {
  /*
   * Get a string representation of an AI_INET or AI_INET6
   * sockaddr. The caller has to free the returned string.
   */
  char *text = malloc(INET6_ADDRSTRLEN);
  struct sockaddr_in6 *address_v6;
  struct sockaddr_in *address_v4;
  if (address->sa_family == AF_INET6) {
      address_v6 = (struct sockaddr_in6 *) address;
      inet_ntop(AF_INET6, &address_v6->sin6_addr, text, INET6_ADDRSTRLEN);
  } else if (address->sa_family == AF_INET) {
      address_v4 = (struct sockaddr_in *) address;
      inet_ntop(AF_INET, &address_v4->sin_addr, text, INET_ADDRSTRLEN);
  } else {
      return ("[Unknown family address]");
  }
  return text;
}
