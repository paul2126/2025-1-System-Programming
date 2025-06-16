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

void process_commands(int sock, int interactive)

{
  // long lVar1;
  int iVar2;
  int *piVar3;
  char *pcVar4;
  // long in_FS_OFFSET;
  // int interactive_local;
  // int sock_local;
  char *line;
  size_t len;
  ssize_t total_sent;
  ssize_t to_send;
  ssize_t total_written;
  ssize_t nread;
  ssize_t rlen;
  ssize_t written;
  ssize_t wlen;
  char buffer[4096];

  // lVar1 = *(long *)(in_FS_OFFSET + 0x28);
  line = (char *)0x0;
  len = 0;
  while (1) {
    if (interactive != 0) {
      printf("Enter command: ");
      fflush(stdout);
    }
    nread = getline(&line, &len, stdin);
    if (nread < 0)
      break;
    if ((nread == 1) && (*line == '\n'))
      goto LAB_00101876;
    total_sent = 0;
    to_send = nread;
    while (total_sent < nread) {
      wlen = write(sock, line + total_sent, to_send);
      if (wlen < 0) {
        piVar3 = __errno_location();
        if (((*piVar3 != 4) &&
             (piVar3 = __errno_location(), *piVar3 != 0xb)) &&
            (piVar3 = __errno_location(), *piVar3 != 0xb)) {
          piVar3 = __errno_location();
          if (*piVar3 == 0x20) {
            fwrite("Connection closed by server\n", 1, 0x1c, stderr);
          } else {
            piVar3 = __errno_location();
            if (*piVar3 == 0x68) {
              fwrite("Connection reset by server\n", 1, 0x1b, stderr);
            } else {
              perror("Send failed");
            }
          }
          free(line);
          goto LAB_00101885;
        }
      } else {
        total_sent = total_sent + wlen;
        to_send = to_send - wlen;
      }
    }
    if (interactive != 0) {
      printf("Server reply: ");
      fflush(stdout);
    }
    do {
      while (rlen = read(sock, buffer, 0x1000), rlen < 0) {
        piVar3 = __errno_location();
        if (((*piVar3 != 4) &&
             (piVar3 = __errno_location(), *piVar3 != 0xb)) &&
            (piVar3 = __errno_location(), *piVar3 != 0xb)) {
          piVar3 = __errno_location();
          if (*piVar3 != 0x68) {
            perror("Recv failed");
            free(line);
            /* WARNING: Subroutine does not return */
            exit(1);
          }
          fwrite("Connection closed by server\n", 1, 0x1c, stderr);
          free(line);
          goto LAB_00101885;
        }
      }
      if (rlen == 0) {
        fwrite("Connection closed by server\n", 1, 0x1c, stderr);
        free(line);
        goto LAB_00101885;
      }
      total_written = 0;
      while (total_written < rlen) {
        written =
            write(1, buffer + total_written, rlen - total_written);
        if (written < 0) {
          piVar3 = __errno_location();
          if (*piVar3 != 4) {
            perror("Write failed");
            free(line);
            goto LAB_00101885;
          }
        } else {
          total_written = total_written + written;
        }
      }
      buffer[total_written] = '\0';
      pcVar4 = strchr(buffer, 10);
    } while (pcVar4 == (char *)0x0);
  }
  iVar2 = feof(stdin);
  if (iVar2 == 0) {
    perror("Error reading input");
  }
LAB_00101876:
  free(line);
LAB_00101885:
  printf("\nExiting...\n");
  // if (lVar1 == *(long *)(in_FS_OFFSET + 0x28)) {
  //   return;
  // }
  /* WARNING: Subroutine does not return */
  // __stack_chk_fail();
}

int main(int argc, char **argv)

{
  // long lVar1;
  int iVar2;
  // char *pcVar3;
  // long in_FS_OFFSET;
  // char **argv_local;
  // int argc_local;
  int sock;
  int port;
  int interactive;
  // int opt;
  // int ret;
  struct addrinfo *res;
  struct addrinfo *rp;
  char *ip;
  struct addrinfo hints;
  char port_str[6];

  // lVar1 = *(long *)(in_FS_OFFSET + 0x28);
  ip = "127.0.0.1";
  port = 0x1f90;
  interactive = 0;
  while (iVar2 = getopt(argc, argv, "i:p:th"), iVar2 != -1) {
    if (iVar2 == 0x74) {
      interactive = 1;
    } else {
      if (0x74 < iVar2) {
      LAB_00101948:
        printf("Usage: %s [-i server_ip_or_domain (%s)] [-p port (%d)] "
               "[-t]\n",
               *argv, "127.0.0.1", 0x1f90);
        /* WARNING: Subroutine does not return */
        exit(1);
      }
      if (iVar2 == 0x69) {
        ip = optarg;
      } else {
        if (iVar2 != 0x70)
          goto LAB_00101948;
        port = atoi(optarg);
        if ((port < 0x401) || (0xffff < port)) {
          perror("Invalid port number");
          /* WARNING: Subroutine does not return */
          exit(1);
        }
      }
    }
  }
  memset(&hints, 0, 0x30);
  hints.ai_family = 0;
  hints.ai_socktype = 1;
  snprintf(port_str, 6, "%ld", (ulong)(uint)port);
  iVar2 = getaddrinfo(ip, port_str, (struct addrinfo *)&hints,
                      (struct addrinfo **)&res);
  if (iVar2 != 0) {
    // pcVar3 = gai_strerror(iVar2);
    // fprintf(stderr, "getaddrinfo failed: %s\n", pcVar3);
    /* WARNING: Subroutine does not return */
    exit(1);
  }
  for (rp = res; rp != (struct addrinfo *)0x0; rp = rp->ai_next) {
    sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sock != -1) {
      iVar2 =
          connect(sock, (struct sockaddr *)rp->ai_addr, rp->ai_addrlen);
      if (iVar2 == 0)
        break;
      iVar2 = close(sock);
      if (iVar2 < 0) {
        perror("Failed to close socket");
        /* WARNING: Subroutine does not return */
        exit(1);
      }
    }
  }
  if (rp == (struct addrinfo *)0x0) {
    fprintf(stderr, "Could not connect to %s:%ld\n", ip,
            (ulong)(uint)port);
    freeaddrinfo((struct addrinfo *)res);
    /* WARNING: Subroutine does not return */
    exit(1);
  }
  freeaddrinfo((struct addrinfo *)res);
  if (interactive != 0) {
    printf("Connected to %s:%ld\n", ip, (ulong)(uint)port);
  }
  process_commands(sock, interactive);
  iVar2 = close(sock);
  if (-1 < iVar2) {
    // if (lVar1 == *(long *)(in_FS_OFFSET + 0x28)) {
    //   return 0;
    // }
    // /* WARNING: Subroutine does not return */
    // __stack_chk_fail();
  }
  perror("Failed to close socket");
  /* WARNING: Subroutine does not return */
  exit(1);
}
