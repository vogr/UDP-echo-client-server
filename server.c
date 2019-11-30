#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 2048
#define MAX_SOCKS 20

int main(int argc, char *argv[]) {
  int ret = 0;

  if (argc < 2) {
    fprintf(stderr, "Error: requires one argument\n\t./server IP_ADDRESS\n");
    return EXIT_FAILURE;
  }


  /*
   * Implement a dual stack echo server using the ideas from
   * https://tools.ietf.org/html/rfc4038#section-6.3
   * i.e. bind to all addresses returned by getaddrinfo (and setting
   * IPV6_V6ONLY to prevent binding conflicts)
   */

  /* From `getaddrinfo` manpage:
   * If the AI_PASSIVE flag is specified in hints.ai_flags, and node is NULL,
   * then the returned socket addresses will be suitable for bind(2)ing a
   * socket that  will  accept(2) connections.
   */
  struct addrinfo hints = {
    .ai_flags = AI_PASSIVE,    /* Bindable IP addresses */
    .ai_family = AF_UNSPEC,
    .ai_socktype = SOCK_DGRAM, /* UDP socket */
  };

  struct addrinfo *addresses_linked_list = NULL;
  ret = getaddrinfo(
      NULL,
      argv[1],
      &hints,
      &addresses_linked_list
  );
  if (ret != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
    exit(1);
  }

  int sockets[MAX_SOCKS] = {0};
  int nsock = 0
  const int on = 1; // for setsockopt IPV6_V6ONLY
  struct addrinfo *address = NULL;
  for (address = addresses_linked_list; address != NULL ; address = address->ai_next) {
    if (nsock >= MAX_SOCKS) {
      fprintf(stderr, "Warning: maximum number of sockets reached, will not try to bind the remaining addresses.\n");
      break;
    }
    sockets[nsock] = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
    if (sockets[nsock] == -1) {
      // could not create socket
      continue;
    }

    /* To prevent conflict with IPv4 adresses, set IPV6_V6ONLY on IPv6 addresses */
    if (adress->ai_family == AF_INET6) {
      ret = setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&on, sizeof(on));
      if (ret != 0) {
        char *address_repr = text_of((struct sockaddr *)address);
        fprintf(stderr, "Warning: could not set IPV6_V6ONLY flag on the socket for the address %s\n", address_repr)
        perror("setsockopt IPV6_V6ONLY");
        free(address_repr);
      }
    }

    ret = bind(udp_socket, address->ai_addr, address->ai_addrlen);
    if (ret != 0) {
      char *address_repr = text_of((struct sockaddr *)address);
      fprintf(stderr, "Could not bind address %s:\n", address_repr);
      perror("bind");
      free(address_repr);
      close(sockets[nsock]);
      continue;
    }
    nsock++;
  }

  if (address == NULL) {
    fprintf(stderr, "Initialization failed: could not bind to any of the addresses found.\n");
    exit(1);
  }

  // The list of addresses is no longer required
  freeaddrinfo(addresses_linked_list);


  struct sockaddr from = {0};
  socklen_t from_len = sizeof(from);
  char buffer[BUFFER_SIZE] = {0};
  ssize_t bytes_read = 0;
  ssize_t bytes_sent = 0;

  while (1) {
    bytes_read = recvfrom(
        udp_socket, buffer, BUFFER_SIZE, 0,
        &from, &from_len
      );
    if (bytes_read < 0) {
      continue;
    }

    fprintf(stderr, "Received bytes! Sending:\n");
    for(ssize_t i = 0; i < bytes_read; i++) {
      fprintf(stderr, "%c", buffer[i]);
    }
    fprintf(stderr, "\nEND\n");


    bytes_sent = sendto(
        udp_socket, buffer, (size_t)bytes_read, 0,
        &from, from_len
      );
    if (bytes_sent == -1) {
      perror("sendto");
    }
    fprintf(stderr, "Received %zd, sent %zd\n", bytes_read, bytes_sent);

  }

  return EXIT_SUCCESS;
}


// La fonction utilitaire suivante vient du blog
// https://www.bortzmeyer.org/bindv6only.html
// Elle permet d'obtenir une représentation des addresses
// IPv4 et IPv6.
char * text_of(struct sockaddr *address)
{
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
