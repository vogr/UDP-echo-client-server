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

#include "client.h"

// Define a small packet size to prevent IP fragmentation
// Packet sizes under 1500 are reasonable. The packet size
// will be equal to the size of the buffer + the size of
// the headers.
// https://blog.cloudflare.com/ip-fragmentation-is-broken/
// gives an interseting analysis of the issue.
#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
  if (argc < 3) {
    fprintf(stderr, "client: requires two arguments\n\t./client <ADDRESS> <PORT_NUMBER>\n");
    return EXIT_FAILURE;
  }

  int udp_socket = get_udp_socket(argv[1], argv[2]);
  if (udp_socket < 0) {
    fprintf(stderr, "Connection failed: could not create UDP socket\n");
  }

  /*
   * Info and inspiration for poll() taken from
   * https://beej.us/guide/bgnet/html/#poll
   *
   */

  // The client expects 
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

  fprintf(stderr, "[client] Enter text below.\n");
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
       * with poll (a fread() might block even if data is available).
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
      ssize_t write_ret = write(
          udp_socket,
          buffer, bytes_to_send
          );
      if (write_ret < 0) {
        fprintf(stderr, "Error: cannot write to UDP socket\n");
        perror("write");
        return EXIT_FAILURE;
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


/*
 * Returns a socket file descriptor able to send UDP data to address
 * `node` at port `service` over UDP.
 *
 * A negative value signals an error.
 *
 */
int get_udp_socket(const char *node, const char *service) {
  struct addrinfo hints = {
    .ai_family = AF_UNSPEC,    /* Allow IPv4 or IPv6 */
    .ai_socktype = SOCK_DGRAM, /* UDP socket */
  };

  struct addrinfo *addresses_linked_list = NULL;
  int ret = getaddrinfo(
      node,
      service,
      &hints,
      &addresses_linked_list
  );
  if (ret != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
    return -1;
  }

  int udp_socket = -1;
  for (struct addrinfo *address = addresses_linked_list; address != NULL; address = address->ai_next) {
    udp_socket = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
    if (udp_socket == -1) {
      // could not create socket
      continue;
    }

    // We use connect() on a UDP socket. Some motivations are written here:
    // http://www.masterraghu.com/subjects/np/introduction/unix_network_programming_v1.3/ch08lev1sec11.html
    if (connect(udp_socket, address->ai_addr, address->ai_addrlen) != -1) {
      // Success. There is no handshake with UDP: a successful connect() simply means that
      // the route lookup was successful and that the fields are set for future read/write (or send/recv).
      break;
    }
    else {
      close(udp_socket);
      udp_socket = -1;
    }
  }

  // Note: if we reach the end of the list (address = NULL),  then udp_socket = -1 will signal the error.

  // The list of addresses is no longer required
  freeaddrinfo(addresses_linked_list);
  return udp_socket;

}
