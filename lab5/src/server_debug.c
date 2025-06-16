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

void *handle_client(void *arg)

{
  // long lVar1;
  int iVar2;
  int *piVar3;
  ssize_t sVar4;
  size_t sVar5;
  // long in_FS_OFFSET;
  // void *arg_local;
  socklen_t socklen;
  int off;
  int idx;
  int listen_fd;
  int client_fd;
  int rlen;
  int wlen;
  size_t wbuf_len;
  size_t rbuf_len;
  struct thread_args *args;
  struct skvs_ctx *ctx;
  struct timeval to;
  struct sockaddr_in client;
  char rbuf[4096];
  char wbuf[4096];

  // lVar1 = *(long *)(in_FS_OFFSET + 0x28);
  ctx = *(struct skvs_ctx **)((long)arg + 8);
  idx = *(int *)((long)arg + 4);
  /* WARNING: Load size is inaccurate */
  args = (struct thread_args *)arg;

  listen_fd = args->listenfd;
  client_fd = -1;
  rbuf_len = 0;
  wbuf_len = 0;
  args = (struct thread_args *)arg;
  free(arg);
  printf("%ldth worker ready\n", (ulong)(uint)idx);
LAB_00102b5e:
  if (g_shutdown == 0) {
    socklen = 0x10;
    client_fd = accept(listen_fd, (struct sockaddr *)&client, &socklen);
    if (client_fd < 0)
      goto code_r0x001027cc;
    to.tv_sec = 1;
    to.tv_usec = 0;
    iVar2 = setsockopt(client_fd, 1, 0x14, &to, 0x10);
    if (iVar2 < 0) {
      perror("setsockopt SO_RCVTIMEO failed");
    } else {
      while (g_shutdown == 0) {
        rbuf_len = 0;
        do {
          while (1) {
            if (g_shutdown != 0)
              goto LAB_001029bb;
            sVar4 = read(client_fd, rbuf + rbuf_len, 0x1000 - rbuf_len);
            rlen = (int)sVar4;
            if (rlen == 0) {
              fwrite("Connection closed by client\n", 1, 0x1c, stderr);
              goto LAB_00102b26;
            }
            if (-1 < rlen)
              break;
            piVar3 = __errno_location();
            if (((*piVar3 != 4) &&
                 (piVar3 = __errno_location(), *piVar3 != 0xb)) &&
                (piVar3 = __errno_location(), *piVar3 != 0xb)) {
              piVar3 = __errno_location();
              if (*piVar3 != 0x68) {
                perror("Failed to receive request");
                /* WARNING: Subroutine does not return */
                exit(1);
              }
              fwrite("Connection closed by client\n", 1, 0x1c, stderr);
              goto LAB_00102b26;
            }
          }
          rbuf_len = rbuf_len + (long)rlen;
          iVar2 = skvs_serve(ctx, rbuf, rbuf_len, wbuf, &wbuf_len);
        } while (iVar2 < 1);
      LAB_001029bb:
        if (g_shutdown != 0)
          break;
        off = 0;
        do {
          while (1) {
            if (g_shutdown != 0)
              goto LAB_00102b0a;
            sVar4 = write(client_fd, wbuf + off, wbuf_len - (long)off);
            wlen = (int)sVar4;
            if (-1 < wlen)
              break;
            piVar3 = __errno_location();
            if (((*piVar3 != 4) &&
                 (piVar3 = __errno_location(), *piVar3 != 0xb)) &&
                (piVar3 = __errno_location(), *piVar3 != 0xb)) {
              piVar3 = __errno_location();
              if (*piVar3 == 0x20) {
                fwrite("Connection closed by client\n", 1, 0x1c,
                       stderr);
              } else {
                piVar3 = __errno_location();
                if (*piVar3 != 0x68) {
                  perror("Failed to send response");
                  /* WARNING: Subroutine does not return */
                  exit(1);
                }
                fwrite("Connection reset by client\n", 1, 0x1b, stderr);
              }
              goto LAB_00102b26;
            }
          }
          off = off + wlen;
          sVar5 = strlen(wbuf);
        } while ((ulong)(long)off < sVar5);
      LAB_00102b0a:
        rbuf_len = 0;
      }
    }
    goto LAB_00102b26;
  }
LAB_00102b6c:
  // if (lVar1 == *(long *)(in_FS_OFFSET + 0x28)) {
  //   return (void *)0x0;
  // }
  /* WARNING: Subroutine does not return */
  // __stack_chk_fail();
code_r0x001027cc:
  piVar3 = __errno_location();
  if (((*piVar3 != 4) &&
       (piVar3 = __errno_location(), *piVar3 != 0xb)) &&
      (piVar3 = __errno_location(), *piVar3 != 0xb)) {
    perror("accept failed");
  LAB_00102b26:
    if (client_fd != -1) {
      iVar2 = close(client_fd);
      if (iVar2 < 0) {
        perror("close failed");
        goto LAB_00102b6c;
      }
      client_fd = -1;
    }
  }
  goto LAB_00102b5e;
}

void handle_sigint(int sig)

{
  // int sig_local;

  puts("\nReceived SIGINT, initiating shutdown...");
  g_shutdown = 1;
  return;
}

int main(int argc, char **argv)

{
  // long lVar1;
  int iVar2;
  __sighandler_t p_Var3;
  // long in_FS_OFFSET;
  // char **argv_local;
  // int argc_local;
  int optval;
  int port;
  int i;
  int num_threads;
  int delay;
  int listen_fd;
  int opt;
  size_t hash_size;
  struct skvs_ctx *ctx;
  char *ip;
  pthread_t *threads;
  struct thread_args *args;
  struct timeval to;
  struct sockaddr_in server;

  // lVar1 = *(long *)(in_FS_OFFSET + 0x28);
  ctx = (struct skvs_ctx *)0x0;
  hash_size = 0x400;
  ip = "0.0.0.0";
  port = 0x1f90;
  num_threads = 10;
  delay = 0;
LAB_00102cf7:
  while (1) {
    opt = getopt(argc, argv, "p:t:s:d:h");
    if (opt == -1) {
      ctx = skvs_init(hash_size, delay);
      if (ctx == (struct skvs_ctx *)0x0) {
        perror("Failed to init SKVS");
        /* WARNING: Subroutine does not return */
        exit(1);
      }
      p_Var3 = signal(2, handle_sigint);
      if (p_Var3 == (__sighandler_t)0xffffffffffffffff) {
        perror("Failed to set signal handler");
        goto LAB_0010306c;
      }
      listen_fd = socket(2, 1, 0);
      if (listen_fd == -1) {
        perror("Could not create socket");
        goto LAB_0010306c;
      }
      optval = 1;
      iVar2 = setsockopt(listen_fd, 1, 2, &optval, 4);
      if (iVar2 != 0) {
        perror("setsockopt SO_REUSEADDR failed");
        goto LAB_0010306c;
      }
      optval = 1;
      iVar2 = setsockopt(listen_fd, 1, 0xf, &optval, 4);
      if (iVar2 != 0) {
        perror("setsockopt SO_REUSEPORT failed");
        goto LAB_0010306c;
      }
      to.tv_sec = 1;
      to.tv_usec = 0;
      iVar2 = setsockopt(listen_fd, 1, 0x14, &to, 0x10);
      if (iVar2 < 0) {
        perror("setsockopt SO_RCVTIMEO failed");
        goto LAB_0010306c;
      }
      server.sin_family = 2;
      server.sin_addr.s_addr = inet_addr(ip);
      server.sin_port = htons((uint16_t)port);
      iVar2 = bind(listen_fd, (struct sockaddr *)&server, 0x10);
      if (iVar2 != 0) {
        perror("bind failed");
        goto LAB_0010306c;
      }
      iVar2 = listen(listen_fd, 0x14);
      if (iVar2 != 0) {
        perror("listen failed");
        goto LAB_0010306c;
      }
      printf("Server listening on %s:%ld\n", ip, (ulong)(uint)port);
      threads = (pthread_t *)calloc((long)num_threads, 8);
      if (threads == (pthread_t *)0x0) {
        perror("calloc failed");
        close(listen_fd);
        /* WARNING: Subroutine does not return */
        exit(1);
      }
      i = 0;
      goto LAB_00103013;
    }
    if (opt != 0x74)
      break;
    num_threads = atoi(optarg);
  }
  if (opt < 0x75) {
    if (opt == 0x73) {
      iVar2 = atoi(optarg);
      hash_size = (size_t)iVar2;
      if (hash_size == 0) {
        perror("Invalid hash size");
        /* WARNING: Subroutine does not return */
        exit(1);
      }
      goto LAB_00102cf7;
    }
    if (opt < 0x74) {
      if (opt == 100) {
        delay = atoi(optarg);
      } else {
        if (opt != 0x70)
          goto LAB_00102cb5;
        port = atoi(optarg);
      }
      goto LAB_00102cf7;
    }
  }
LAB_00102cb5:
  // printf("Usage: %s [-p port (%d)] [-t num_threads (%d)] [-d "
  //        "rwlock_delay (%d)] [-s hash_size (%d)]\ n",
  //        *argv, 0x1f90, 10, 0, 0x400);
  /* WARNING: Subroutine does not return */
  // exit(1);
LAB_00103013:
  if (num_threads <= i) {
    for (i = 0; i < num_threads; i = i + 1) {
      iVar2 = pthread_join(threads[i], (void **)0x0);
      if (iVar2 != 0) {
        perror("Failed to join thread");
      }
    }
  LAB_0010306c:
    puts("Shutting down server...");
    if (-1 < listen_fd) {
      close(listen_fd);
    }
    if (ctx != (struct skvs_ctx *)0x0) {
      iVar2 = skvs_destroy(ctx, 1);
      if (iVar2 < 0) {
        perror("Failed to destroy SKVS");
      }
    }
    // if (lVar1 != *(long *)(in_FS_OFFSET + 0x28)) {
    //   /* WARNING: Subroutine does not return */
    //   // __stack_chk_fail();
    // }
    return 0;
  }
  args = (struct thread_args *)calloc(1, 0x10);
  if (args == (struct thread_args *)0x0) {
    perror("calloc failed");
    close(listen_fd);
    goto LAB_0010306c;
  }
  args->idx = i;
  args->ctx = ctx;
  args->listenfd = listen_fd;
  iVar2 = pthread_create(threads + i, (pthread_attr_t *)0x0,
                         handle_client, args);
  if (iVar2 != 0) {
    perror("Could not create thread");
    free(args);
    goto LAB_0010306c;
  }
  i = i + 1;
  goto LAB_00103013;
}
