# UDP Echo client and server

## Introduction

This project implements two parts of a networked system: a server and a client communicating over UDP.

* the client sends each line that the user enters on the standard input to the server over UDP. Additionally, it prints to the standard output the data that it receives from the server.
* the server waits for UDP input, and sends back any data it receives to the sender. Additionally, it will provide logging information on the standard error stream.

We can summarize the data flow like this:

> (client stdin) --> client --[UDP]--> server --[UDP]--> client --> (client stdout)

The client and server do not check that the UDP packets have been actually received on the other end ; the user should not expect reliability from the transmission.

## Compilation

A Makefile is provided. A call to `make` will build the two executables (`client` and `server`).

Debug builds can be obtained by building the executables directly in the source directories like so:
``` console
$ cd src_client
$ make BUILD=debug
```
The resulting file will be called `client.debug`.

## Usage

### Client

```
client <address> <port>
```

The client will prompt the user for input: each line of text the user inputs on stdin will be sent to the server at `<address>` on `<port>` over UDP. If the server at `<address>` sends back data, it will be printed on the standard output.

### Server
```
server <port>
```

The server will wait for UDP data on the port `<port>`. When data arrive from a sender over UDP, it will send back the data to the sender over UDP.

## Example

* Client:
``` console
$ ./client "129.104.254.41" "8000"
[client] Enter text below.
Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.
Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.
I just want to tell you both good luck. We’re all counting on you.
I just want to tell you both good luck. We’re all counting on you.
If you've heard this story before, don't stop me, because I'd like to hear it again.
If you've heard this story before, don't stop me, because I'd like to hear it again.
```
Note: the 3rd, 5th and 7th line were written by the user on stdin, the 4th, 6th and 8th line were printed by the client upon receiving data from the server.

* Server:
```console
$ ./server "8000"
Listening on port 8000
[server] Received bytes from ::ffff:129.104.241.126, will send them back. Message received:
Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.\n
[server] Received bytes from ::ffff:129.104.241.126, will send them back. Message received:
I just want to tell you both good luck. We’re all counting on you.\n
[server] Received bytes from ::ffff:129.104.241.126, will send them back. Message received:
If you've heard this story before, don't stop me, because I'd like to hear it again.\n
```

## Technical notes

* The client is able to send packets over IPv4 and IPv6. The preferred protocol is not defined however, the first successful connection using an address returned by `getaddrinfo` will determine the protocol used.
* The client uses `poll()` to monitor both stdin and the UDP socket. When any of these two streams can be read, it processes the new data.
* The server uses a single IPv6 socket to communicate with both IPv4 and IPv6 clients. It relies of the in-kernel mapping mechanism which presents IPv4 addresses as IPv6 addresses on the socket. See [RFC 3493 §3.7](https://tools.ietf.org/html/rfc3493#section-3.7) and [RFC 3493 §5.3](https://tools.ietf.org/html/rfc3493#section-5.3) for details.

