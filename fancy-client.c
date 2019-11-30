#include <netdb.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>


#define BUFFER_SIZE 2048

int main(int argc, char *argv[]) {
  int ret = 0;

  if (argc < 3) {
    fprintf(stderr, "Error: requires two arguments\n\t./client IP_ADDRESS PORT_NUMBER\n");
    return EXIT_FAILURE;
  }

  struct addrinfo hints = {
    .ai_family = AF_UNSPEC,    /* Allow IPv4 or IPv6 */
    .ai_socktype = SOCK_DGRAM, /* UDP socket */
  };

  struct addrinfo *addresses_linked_list = NULL;
  ret = getaddrinfo(
      argv[1],
      argv[2],
      &hints,
      &addresses_linked_list
  );
  if (ret != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
    exit(1);
  }

  struct addrinfo *address = NULL;
  int udp_socket = -1;
  for (address = addresses_linked_list; address != NULL; address = address->ai_next) {
    udp_socket = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
    if (udp_socket == -1) {
      // could not create socket
      continue;
    }

    // We use connect() on a UDP socket. Some motivations are written here:
    // http://www.masterraghu.com/subjects/np/introduction/unix_network_programming_v1.3/ch08lev1sec11.html
    // By connect()ing, we check that everything is correct and simplify the
    // calls to send and recv (instead of sendto and recvfrom).
    if (connect(udp_socket, address->ai_addr, address->ai_addrlen) != -1) {
      // Connection successful! (There is no handshake with UDP: "connection" simply means that
      // the fields are set for future read/write (or send/recv).
      break;
    }
    else {
      close(udp_socket);
      udp_socket = -1;
    }
  }

  if (address == NULL) {
    fprintf(stderr, "Connection failed: could not use any of the addresses found.\n");
    exit(1);
  }

  // The list of addresses is no longer required
  freeaddrinfo(addresses_linked_list);


  char buffer[BUFFER_SIZE] = {0};
  size_t bytes_to_send = 0;
  ssize_t bytes_sent = 0;
  ssize_t bytes_received_ssize_t = 0;
  size_t bytes_received = 0;
  /*
   * /!\ This implementation ignores parts of the input if it contains a null-terminator!
   *
   */
  fprintf(stderr, "> ");
  while (fgets(buffer, BUFFER_SIZE, stdin) != NULL) {
    bytes_to_send = strlen(buffer); //do not include the null-terminator added by fgets
    while (bytes_to_send > 0) {
      bytes_sent = write(
          udp_socket,
          buffer, bytes_to_send
          );
      if (bytes_sent < 0) {
        fprintf(stderr, "Error: cannot write to UDP socket\n");
        perror("write");
        exit(1);
      }
      bytes_to_send -= (size_t)bytes_sent;
    }


    //fprintf(stderr,"Blocking for read...\n");
    bytes_received_ssize_t = read(udp_socket, buffer, BUFFER_SIZE);
    //fprintf(stderr,"Read done.\n");
    if (bytes_received_ssize_t < 0) {
      fprintf(stderr, "Error: cannot read from UDP socket\n");
      perror("read");
      exit(1);
    }
    bytes_received = (size_t)bytes_received_ssize_t;

    for (size_t i = 0; i < bytes_received; i++) {
      printf("%c", buffer[i]);
    }
    fprintf(stderr, "> ");
  }

  return EXIT_SUCCESS;
}
