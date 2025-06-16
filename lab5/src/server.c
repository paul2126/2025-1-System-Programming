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

  // loop the entire thread
  for (;;) {
    if (g_shutdown) {
      break;
    }
    // printf("%d\n", errno);
    // accept client connection
    struct sockaddr_in client_addr;
    socklen_t addrlen = sizeof client_addr;
    int client_fd =
        accept(listenfd, (struct sockaddr *)&client_addr, &addrlen);
    if (client_fd < 0) {
      if (errno == EINTR || errno == EAGAIN)
        continue;
      perror("accept failed");
      break;
    }

    // set timeout for client socket. prevent blocking forever
    struct timeval tv0 = {1, 0};
    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &tv0, sizeof(tv0));

    // persistent connection with one client
    for (;;) {
      if (g_shutdown) {
        break;
      }
      char rbuf[BUFFER_SIZE];
      char wbuf[BUFFER_SIZE];
      size_t wbuf_len;
      ssize_t rbuf_len;
      size_t total_recv = 0;

      // receive data
      while (!g_shutdown) {
        rbuf_len = recv(client_fd, rbuf + total_recv,
                        BUFFER_SIZE - total_recv, 0);
        if (rbuf_len < 0) {
          if (errno == EAGAIN || errno == EINTR) { // timeout
            continue;
          }
          if (errno == ECONNRESET) {
            goto close_and_continue;
          }
          // unknown error close connection
          perror("recv");
          goto close_and_continue;
          exit(EXIT_FAILURE);
        } else if (rbuf_len == 0) { // client disconnected
          // printf("client disconnected.\n");
          goto close_and_continue;
        }
        total_recv += rbuf_len;
        // end of message
        if (rbuf[total_recv - 1] == '\n') {
          break;
        }
      }
      if (total_recv == 1 && rbuf[0] == '\n') {
        // empty message close connection
        goto close_and_continue;
      }

      // parse request
      int is_done = skvs_serve(ctx, rbuf, rbuf_len, wbuf, &wbuf_len);

      if (g_shutdown) {
        goto close_and_continue;
      }

      // loop for sending response
      if (is_done) { // parse success
        ssize_t total_sent = 0;

        while (total_sent < wbuf_len && !g_shutdown) {
          ssize_t sent = send(client_fd, wbuf + total_sent,
                              wbuf_len - total_sent, 0);
          if (sent < 0) { // send error
            if (errno == EAGAIN || errno == EINTR) {
              continue;
            }
            if (errno == EPIPE) {
              goto close_and_continue;
            }
            if (errno == ECONNRESET) {
              goto close_and_continue;
            }
            perror("send");
            exit(EXIT_FAILURE);
          } else { // sent successfully
            total_sent += sent;
          }
        }
      } else { // parse error
        perror("skvs_serve");
        break;
      }
    }
  close_and_continue:
    close(client_fd);
    printf("Connection closed by client\n");
  }
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

  // printf("Server started on %s:%d with %d threads, "
  //        "hash size: %zu, rwlock delay: %d\n",
  //        ip, port, num_threads, hash_size, delay);

  // register signal handler for SIGINT
  // if (signal(SIGINT, handle_sigint) == SIG_ERR) {
  //   perror("signal");
  //   exit(EXIT_FAILURE);
  // }
  struct sigaction sa = {.sa_handler = handle_sigint};
  sigemptyset(&sa.sa_mask);
  if (sigaction(SIGINT, &sa, NULL) == -1) {
    perror("signal");
    exit(EXIT_FAILURE);
  }
  struct addrinfo hints, *res, *rp;
  memset(&hints, 0, sizeof hints);

  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  char port_str[6]; // enough for 5-digit port + null
  snprintf(port_str, sizeof(port_str), "%d", port);
  if (getaddrinfo(ip, port_str, &hints, &res) < 0) {
    perror("getaddrinfo");
    return EXIT_FAILURE;
  }

  // create bind
  for (rp = res; rp != NULL; rp = rp->ai_next) {
    if (bind(listen_fd, rp->ai_addr, rp->ai_addrlen) == 0)
      break;

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
  printf("Server listening on %s:%d\n", ip, port);

  // init skvs
  struct skvs_ctx *ctx = skvs_init(hash_size, delay);

  // create workers
  pthread_t *workers = calloc(num_threads, sizeof *workers);
  for (int i = 0; i < num_threads; i++) {
    struct thread_args *ta = malloc(sizeof *ta);
    ta->listenfd = listen_fd; // shared listen_fd
    ta->idx = i;
    ta->ctx = ctx; // HT
    if (pthread_create(&workers[i], NULL, handle_client, ta) != 0) {
      perror("pthread_create");
      exit(EXIT_FAILURE);
    }
  }

  // wait for workers to finish
  for (int i = 0; i < num_threads; i++) {
    // printf("waiting worker %d to finish\n", i);
    // printf("g_shutdown: %d\n", g_shutdown);
    pthread_join(workers[i], NULL);
  }
  close(listen_fd);
  skvs_destroy(ctx, 1);
  free(workers);

  // printf("server terminated\n");
  return EXIT_SUCCESS;
  /*---------------------------------------------------------------------------*/

  return 0;
}
