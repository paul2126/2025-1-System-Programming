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
  // printf("%d fd: %d\n", idx, listenfd);
  while (!g_shutdown) {
    // accept
    struct sockaddr_storage client_addr;
    socklen_t addrlen = sizeof client_addr;
    int client_fd =
        accept(listenfd, (struct sockaddr *)&client_addr, &addrlen);
    if (client_fd == -1) {
      // printf("accept failed: %s\n", strerror(errno));
      if (errno == EINTR) {
        perror("accept interrupted");
        continue;
      }
      if (errno == EBADF || errno == ENOTSOCK) {
        // socket closed
        // printf("worker %d: socket closed exiting\n", idx);
        break;
      }
      if (!g_shutdown) {
        // printf("worker %d: accept fail: %s\n", idx, strerror(errno));
      }
      continue;
    }

    // set timeout for client socket. prevent blocking forever
    struct timeval tv0 = {1, 0};
    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &tv0, sizeof(tv0));

    int is_connected = 1;
    while (!g_shutdown && is_connected) { // handle client
      char rbuf[BUFFER_SIZE + 1];
      char wbuf[BUFFER_SIZE + 1];
      size_t wbuf_len;
      ssize_t rbuf_len;
      size_t total_recv = 0;

      // receive data
      while (!g_shutdown && total_recv < BUFFER_SIZE) {
        rbuf_len = recv(client_fd, rbuf + total_recv,
                        BUFFER_SIZE - total_recv, 0);
        if (rbuf_len < 0) {
          if (errno == EAGAIN || errno == EWOULDBLOCK) { // timeout
            continue;
          }
          if (g_shutdown) { // check g_shutdown
            // printf("worker %d: shutdown detected, closing client\n",
            //        idx);
            return NULL;
          }
          // unknown error close connection
          is_connected = 0;
          perror("recv");
          break;
        } else if (rbuf_len == 0) {
          // client disconnected
          // printf("client disconnected.\n");
          is_connected = 0;
          break;
        }
        total_recv += rbuf_len;
        // end of message
        if (rbuf[total_recv - 1] == '\n') {
          break;
        }
      }
      // printf("received %zd bytes from client.\n", rbuf_len);

      // parse request
      skvs_serve(ctx, rbuf, rbuf_len, wbuf, &wbuf_len);
      if (wbuf_len > 0 && !g_shutdown && rbuf_len > 0) {
        // send response
        ssize_t total_sent = 0;

        int is_sent = 1;
        while (total_sent < wbuf_len && !g_shutdown && is_sent) {
          ssize_t sent = send(client_fd, wbuf + total_sent,
                              wbuf_len - total_sent, 0);
          if (sent < 0) { // send error
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
              continue;
            }
            perror("send");
            is_sent = 0;      // error stop send
            is_connected = 0; // stop client connection
            break;
          } else { // sent successfully
            total_sent += sent;
          }
        }
      } else if (wbuf_len < 0) { // serve error
        perror("skvs_serve");
        is_connected = 0; // stop client connection
        break;
      }
    }
    close(client_fd);
    // printf("client disconnected.\n");
  }
  // printf("Worker %d exiting...\n", idx);

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

  // block SIGINT in all threads, handle it only in main
  sigset_t sigset;
  sigemptyset(&sigset);
  sigaddset(&sigset, SIGINT);
  if (pthread_sigmask(SIG_BLOCK, &sigset, NULL) != 0) {
    perror("pthread_sigmask");
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

  // wait for sigint in main
  int signum;
  if (sigwait(&sigset, &signum) != 0) {
    perror("sigwait");
    exit(EXIT_FAILURE);
  }
  g_shutdown = 1; // set shutdown flag
  close(listen_fd);

  // wait for workers to finish
  for (int i = 0; i < num_threads; i++) {
    // printf("waiting worker %d to finish\n", i);
    // printf("g_shutdown: %d\n", g_shutdown);
    pthread_join(workers[i], NULL);
  }
  skvs_destroy(ctx, 1);
  free(workers);

  // printf("server terminated\n");
  return EXIT_SUCCESS;
  /*---------------------------------------------------------------------------*/

  return 0;
}
