#include <netdb.h>
#include <poll.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>


#define BUFFER_SIZE 4096

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


  /*
   * Info and inspiration for poll() taken from
   * https://beej.us/guide/bgnet/html/#poll
   *
   * /!\ This implementation ignores parts of the input if it contains a null-terminator!
   *
   */
  struct pollfd pfds[] =
    {
      {.fd = STDIN_FILENO, .events = POLLIN},    // stdin
      {.fd = udp_socket, .events= POLLIN},       // UDP socket
    };
  nfds_t nfds = 2;

  /*
   * /!\ EOF is always readable and is not discarded by a read()!
   * This means that we should check for EOF, or else poll() will
   * never block after an EOF.
   * see: https://stackoverflow.com/a/18617028
   */

  char buffer[BUFFER_SIZE] = {0};
  while (1) {
    int num_events = poll(pfds, nfds, -1);
    if (num_events == -1) {
      perror("poll");
      return EXIT_FAILURE;
    }
    if (pfds[0].revents & (POLLHUP | POLLERR | POLLNVAL)) {
      fprintf(stderr, "Connection closed: stdin.\n");
      return EXIT_FAILURE;
    }
    if (pfds[0].revents & POLLIN) {
      // stdin is ready for read

      /* fgets() and fread() buffer the input ; this does not work very well
       * with poll (the data is not printed as soon as it is received).
       * We will use read().
       */
      ssize_t read_ret = read(STDIN_FILENO, buffer, BUFFER_SIZE);
      if (read_ret < 0) {
        fprintf(stderr, "Error: cannot read from stdin\n");
        perror("read");
        return EXIT_FAILURE;
      }
      if (read_ret == 0) {
        fprintf(stderr, "Received EOF from stdin.\n");
        return EXIT_SUCCESS;
      }
      size_t bytes_to_send = (size_t)read_ret;
      size_t offset = 0;
      while (bytes_to_send > 0) {
        ssize_t bytes_sent = write(
            udp_socket,
            buffer + offset, bytes_to_send
            );
        if (bytes_sent < 0) {
          fprintf(stderr, "Error: cannot write to UDP socket\n");
          perror("write");
          return EXIT_FAILURE;
        }
        bytes_to_send -= (size_t)bytes_sent;
        offset += (size_t)bytes_sent;
      }
    }
    if (pfds[1].revents & (POLLHUP | POLLERR | POLLNVAL)) {
      fprintf(stderr, "Connection closed: UDP socket.\n");
      return EXIT_FAILURE;
    }
    if (pfds[1].revents & POLLIN) {
      // udp_socket is ready for read
      ssize_t bytes_received = read(udp_socket, buffer, BUFFER_SIZE);
      if (bytes_received < 0) {
        fprintf(stderr, "Error: cannot read from UDP socket\n");
        perror("read");
        return EXIT_FAILURE;
      }
      if (bytes_received == 0) {
        fprintf(stderr, "Received EOF from UDP socket.\n");
        return EXIT_FAILURE;
      }

      for (ssize_t i = 0; i < bytes_received; i++) {
        printf("%c", buffer[i]);
      }
      // The data received may not end with a newline.
      // Use fflush() to print any remaining chars in the buffer now.
      fflush(stdout);
    }
  }

  return EXIT_SUCCESS;
}
