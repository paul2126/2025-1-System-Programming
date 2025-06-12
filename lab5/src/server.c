/*---------------------------------------------------------------------------*/
/* server.c */
/* Author: Junghan Yoon, KyoungSoo Park */
/* Modified by: RyuMyungHyun */
/*---------------------------------------------------------------------------*/
#define _GNU_SOURCE
#include "common.h"
#include "skvslib.h"
#include <arpa/inet.h>
#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
/*---------------------------------------------------------------------------*/
struct thread_args {
  int listenfd;
  int idx;
  struct skvs_ctx *ctx;

  /*---------------------------------------------------------------------------*/
  /* free to use */

  /*---------------------------------------------------------------------------*/
};
/*---------------------------------------------------------------------------*/
volatile static sig_atomic_t g_shutdown = 0;
/*---------------------------------------------------------------------------*/
void *handle_client(void *arg) {
  TRACE_PRINT();
  struct thread_args *args = (struct thread_args *)arg;
  struct skvs_ctx *ctx = args->ctx;
  int idx = args->idx;
  int listenfd = args->listenfd;
  /*---------------------------------------------------------------------------*/
  /* free to declare any variables */

  /*---------------------------------------------------------------------------*/

  free(args);
  printf("%dth worker ready\n", idx);

  /*---------------------------------------------------------------------------*/
  /* edit here */
  printf("%d fd: %d\n", idx, listenfd);

  /*---------------------------------------------------------------------------*/

  return NULL;
}
/*---------------------------------------------------------------------------*/
/* Signal handler for SIGINT */
void handle_sigint(int sig) {
  TRACE_PRINT();
  printf("\nReceived SIGINT, initiating shutdown...\n");
  g_shutdown = 1;
}
/*---------------------------------------------------------------------------*/
int main(int argc, char *argv[]) {
  size_t hash_size = DEFAULT_HASH_SIZE;
  char *ip = DEFAULT_ANY_IP;
  int port = DEFAULT_PORT, opt;
  int num_threads = NUM_THREADS;
  int delay = RWLOCK_DELAY;
  int listen_fd, optval;
  struct timeval to;
  /*---------------------------------------------------------------------------*/
  /* free to declare any variables */

  /*---------------------------------------------------------------------------*/

  /* parse command line options */
  while ((opt = getopt(argc, argv, "p:t:s:d:h")) != -1) {
    switch (opt) {
    case 'p':
      port = atoi(optarg);
      break;
    case 't':
      num_threads = atoi(optarg);
      break;
    case 's':
      hash_size = atoi(optarg);
      if (hash_size <= 0) {
        perror("Invalid hash size");
        exit(EXIT_FAILURE);
      }
      break;
    case 'd':
      delay = atoi(optarg);
      break;
    case 'h':
    default:
      printf("Usage: %s [-p port (%d)] "
             "[-t num_threads (%d)] "
             "[-d rwlock_delay (%d)] "
             "[-s hash_size (%d)]\n",
             argv[0], DEFAULT_PORT, NUM_THREADS, RWLOCK_DELAY,
             DEFAULT_HASH_SIZE);
      exit(EXIT_FAILURE);
    }
  }

  /* set listen fd with Reuse addr, port, and timeout options. */
  listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd == -1) {
    perror("Could not create socket");
    return -1;
  }

  optval = 1;
  if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval,
                 sizeof(optval)) != 0) {
    perror("setsockopt SO_REUSEADDR failed");
    close(listen_fd);
    return -1;
  }
  optval = 1;
  if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEPORT, &optval,
                 sizeof(optval)) != 0) {
    perror("setsockopt SO_REUSEPORT failed");
    close(listen_fd);
    return -1;
  }

  /* set timeout for client socket for escape on SIGINT */
  to.tv_sec = TIMEOUT;
  to.tv_usec = 0;
  if (setsockopt(listen_fd, SOL_SOCKET, SO_RCVTIMEO, &to, sizeof(to)) <
      0) {
    perror("setsockopt SO_RCVTIMEO failed");
    close(listen_fd);
    return -1;
  }

  /*---------------------------------------------------------------------------*/
  /* edit here */

  printf("Server started on %s:%d with %d threads, "
         "hash size: %zu, rwlock delay: %d\n",
         ip, port, num_threads, hash_size, delay);

  struct addrinfo hints, *res, *rp;
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  char port_str[6]; // enough for 5-digit port + null
  snprintf(port_str, sizeof(port_str), "%d", port);
  if (getaddrinfo(NULL, port_str, &hints, &res) < 0) {
    perror("getaddrinfo");
    return EXIT_FAILURE;
  }

  // create bind
  for (rp = res; rp != NULL; rp = rp->ai_next) {
    if (bind(listen_fd, rp->ai_addr, rp->ai_addrlen) == 0)
      break; /* success */

    close(listen_fd);
    listen_fd = -1;
  }
  freeaddrinfo(res);

  // check bind result
  if (listen_fd == -1) {
    perror("bind");
    return EXIT_FAILURE;
  }

  // listen
  if (listen(listen_fd, 10) == -1) {
    perror("listen");
    close(listen_fd);
    return EXIT_FAILURE;
  }

    while (1) {
    // accept
    struct sockaddr_storage client_addr;
    socklen_t addrlen = sizeof client_addr;
    int client_fd =
        accept(listen_fd, (struct sockaddr *)&client_addr, &addrlen);
    if (client_fd == -1) {
      if (errno == EINTR)
        break; /* interrupted by Ctrl-C */
      perror("accept");
      continue;
    }

    /* Print remote address */
    char host[NI_MAXHOST], serv[NI_MAXSERV];
    if (getnameinfo((struct sockaddr *)&client_addr, addrlen, host,
                    sizeof host, serv, sizeof serv,
                    NI_NUMERICHOST | NI_NUMERICSERV) == 0) {
      printf("Client connected: %s:%s\n", host, serv);
    }

    while (1) {
      /* Echo loop */
      char buf[10];
      ssize_t n;
      while ((n = recv(client_fd, buf, sizeof buf, 0)) > 0) {
        if (send(client_fd, buf, n, 0) != n) {
          perror("send");
          break;
        }
      }
      if (n == -1)
        perror("recv");
    }

    close(client_fd);
    printf("Client disconnected.\n");
  }

  /* 5. Cleanup */
  close(listen_fd);
  printf("Server terminated.\n");
  return EXIT_SUCCESS;
  /*---------------------------------------------------------------------------*/

  return 0;
}
