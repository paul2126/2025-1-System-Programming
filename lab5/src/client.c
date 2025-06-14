/*---------------------------------------------------------------------------*/
/* client.c */
/* Author: Junghan Yoon, KyoungSoo Park */
/* Modified by: RyuMyungHyun */
/*---------------------------------------------------------------------------*/
#define _GNU_SOURCE
#include "common.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
/*---------------------------------------------------------------------------*/
int main(int argc, char *argv[]) {
  char *ip = DEFAULT_LOOPBACK_IP;
  int port = DEFAULT_PORT;
  // int interactive = 0; /* Default is non-interactive mode */
  int opt;

  /*---------------------------------------------------------------------------*/
  /* free to declare any variables */

  /*---------------------------------------------------------------------------*/

  /* parse command line options */
  while ((opt = getopt(argc, argv, "i:p:th")) != -1) {
    switch (opt) {
    case 'i':
      ip = optarg;
      break;
    case 'p':
      port = atoi(optarg);
      if (port <= 1024 || port >= 65536) {
        perror("Invalid port number");
        exit(EXIT_FAILURE);
      }
      break;
    case 't':
      // interactive = 1;
      break;
    case 'h':
    default:
      printf("Usage: %s [-i server_ip_or_domain (%s)] "
             "[-p port (%d)] [-t]\n",
             argv[0], DEFAULT_LOOPBACK_IP, DEFAULT_PORT);
      exit(EXIT_FAILURE);
    }
  }

  /*---------------------------------------------------------------------------*/
  /* edit here */

  // printf("connecting to server at %s:%d\n", ip, port);
  struct addrinfo hints, *res, *rp;
  int conn_fd;
  memset(&hints, 0, sizeof(hints));
  memset(&res, 0, sizeof(res));
  memset(&rp, 0, sizeof(rp));

  // set socket
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  // resolve address
  char port_str[6];
  snprintf(port_str, sizeof(port_str), "%d", port);
  if (getaddrinfo(ip, port_str, &hints, &res) < 0) {
    perror("getaddrinfo failed");
    exit(EXIT_FAILURE);
  }

  // try each result until we successfully connect
  for (rp = res; rp != NULL; rp = rp->ai_next) {
    conn_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (conn_fd == -1) // no socket made
      continue;

    if (connect(conn_fd, rp->ai_addr, rp->ai_addrlen) == 0)
      break; // connection success

    close(conn_fd); // try next
  }

  if (rp == NULL) {
    perror("no connection");
    freeaddrinfo(res);
    exit(EXIT_FAILURE);
  }

  // printf("connected to server\n");

  char buffer[BUFFER_SIZE + 1];

  // test sending message
  while (1) {
    // send message
    if (!fgets(buffer, BUFFER_SIZE + 1, stdin))
      break;

    if (strncmp(buffer, "quit", 4) == 0)
      break;

    if (send(conn_fd, buffer, strlen(buffer), 0) < 0) {
      perror("send");
      break;
    }

    // receive response
    ssize_t len = recv(conn_fd, buffer, BUFFER_SIZE + 1, 0);
    if (len < 0) {
      perror("recv");
      break;
    } else if (len == 0) {
      printf("server closed\n");
      break;
    }

    buffer[len] = '\0'; // null-terminate
    // server response
    printf("%s", buffer);
  }
  freeaddrinfo(res); // clean up
  close(conn_fd);

  /*---------------------------------------------------------------------------*/

  return 0;
}
