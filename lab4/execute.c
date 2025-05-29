/*---------------------------------------------------------------------------*/
/* execute.c */
/* Author: Jongki Park, Kyoungsoo Park */
/*---------------------------------------------------------------------------*/

#include "execute.h"
#include "dynarray.h"
#include "job.h"
#include "lexsyn.h"
#include "snush.h"
#include "token.h"
#include "util.h"

// extern struct job_manager *manager;
/*---------------------------------------------------------------------------*/
void redout_handler(char *fname) {
  //
  // TODO: redout_handler() in execute.c
  //

  //
  // TODO: redout_handler() in execute.c
  //
}
/*---------------------------------------------------------------------------*/
void redin_handler(char *fname) {
  int fd;

  fd = open(fname, O_RDONLY);
  if (fd < 0) {
    error_print(NULL, PERROR);
    exit(EXIT_FAILURE);
  } else {
    dup2(fd, STDIN_FILENO);
    close(fd);
  }
}
/*---------------------------------------------------------------------------*/
int build_command_partial(DynArray_T oTokens, int start, int end,
                          char *args[]) {
  int i, ret = 0, redin = FALSE, redout = FALSE, cnt = 0;
  struct Token *t;

  /* Build command */
  for (i = start; i < end; i++) {

    t = dynarray_get(oTokens, i);

    if (t->token_type == TOKEN_WORD) {
      if (redin == TRUE) {
        redin_handler(t->token_value);
        redin = FALSE;
      } else if (redout == TRUE) {
        redout_handler(t->token_value);
        redout = FALSE;
      } else
        args[cnt++] = t->token_value;
    } else if (t->token_type == TOKEN_REDIN)
      redin = TRUE;
    else if (t->token_type == TOKEN_REDOUT)
      redout = TRUE;
  }
  args[cnt] = NULL;

#ifdef DEBUG
  for (i = 0; i < cnt; i++) {
    if (args[i] == NULL)
      printf("CMD: NULL\n");
    else
      printf("CMD: %s\n", args[i]);
  }
  printf("END\n");
#endif
  return ret;
}
/*---------------------------------------------------------------------------*/
int build_command(DynArray_T oTokens, char *args[]) {
  return build_command_partial(oTokens, 0, dynarray_get_length(oTokens),
                               args);
}
/*---------------------------------------------------------------------------*/
void execute_builtin(DynArray_T oTokens, enum BuiltinType btype) {
  int ret;
  char *dir = NULL;
  struct Token *t1;

  switch (btype) {
  case B_EXIT:
    if (dynarray_get_length(oTokens) == 1) {
      // printf("\n");
      dynarray_map(oTokens, free_token, NULL);
      dynarray_free(oTokens);

      exit(EXIT_SUCCESS);
    } else
      error_print("exit does not take any parameters", FPRINTF);

    break;

  case B_CD:
    if (dynarray_get_length(oTokens) == 1) {
      dir = getenv("HOME");
      if (dir == NULL) {
        error_print("cd: HOME variable not set", FPRINTF);
        break;
      }
    } else if (dynarray_get_length(oTokens) == 2) {
      t1 = dynarray_get(oTokens, 1);
      if (t1->token_type == TOKEN_WORD)
        dir = t1->token_value;
    }

    if (dir == NULL) {
      error_print("cd takes one parameter", FPRINTF);
      break;
    } else {
      ret = chdir(dir);
      if (ret < 0)
        error_print(NULL, PERROR);
    }
    break;

  default:
    error_print("Bug found in execute_builtin", FPRINTF);
    exit(EXIT_FAILURE);
  }
}
/*---------------------------------------------------------------------------*/
void wait_fg(int jobid) {
  while (1) {
    struct job *job = find_job_by_jid(jobid);

    if (job == NULL) {
      break;
    }

    if (job->state != foreground) {
      break;
    }
    sleep(0);
  }

  return;
}
/*---------------------------------------------------------------------------*/
void print_job(int jobid, pid_t pgid) {
  fprintf(stdout, "[%d] Process group: %d running in the background\n",
          jobid, pgid);
}
/*---------------------------------------------------------------------------*/
int fork_exec(DynArray_T oTokens, int is_background) {
  char *args[MAX_ARGS_CNT];
  build_command(oTokens, args);

  // create job
  int job_id = 0;
  if (is_background) { // 1: background
    job_id = add_job(background);
  } else { // 0: foreground
    job_id = add_job(foreground);
  }

  pid_t pid = fork();
  if (pid < 0) {
    perror("fork failed");
    exit(EXIT_FAILURE);
  }

  if (pid == 0) { // child
    // set pgid
    if (setpgid(0, 0) < 0) {
      perror("setpgid failed");
      exit(EXIT_FAILURE);
    }

    execvp(args[0], args);
    error_print(args[0], PERROR);
    exit(EXIT_FAILURE);
  } else { // parent
    // set pgid of child
    // if (setpgid(pid, pid) < 0 && errno != EACCES) {
    //   perror("setpgid failed");
    //   exit(EXIT_FAILURE);
    // }

    // set element of job
    struct job *job = find_job_by_jid(job_id);
    job->pgid = pid;
    job->pid_list = malloc(sizeof(pid_t));
    job->pid_list[0] = pid;
    job->total_num = 1;
    job->curr_num = 1;

    if (is_background) {
      print_job(job_id, pid);
    } else {
      wait_fg(job_id);
    }
  }

  return job_id;
}
/*---------------------------------------------------------------------------*/
void find_pipe_loc(DynArray_T oTokens, int *start, int *end) {
  *start = *end;
  if (*start != 0) { // not first cmd
    *start += 1;     // skip pipe
  }
  for (int i = *start; i < dynarray_get_length(oTokens); i++) {
    struct Token *t = dynarray_get(oTokens, i);
    if (t->token_type == TOKEN_PIPE) {
      *end = i;
      return;
    }
  }
  *end = dynarray_get_length(oTokens); // if no pipe, set to end
}
/*---------------------------------------------------------------------------*/
void print_fd_status(int save_fd, int pipe_fd[2]) {
  if (fcntl(save_fd, F_GETFD) != -1)
    fprintf(stderr, "Child: save_fd=%d is OPEN before assignment\n",
            save_fd);
  else
    fprintf(stderr, "Child: save_fd=%d is CLOSED or invalid\n",
            save_fd);
  if (fcntl(pipe_fd[0], F_GETFD) != -1)
    fprintf(stderr, "Child: pipe_fd[0]=%d is OPEN before assignment\n",
            pipe_fd[0]);
  else
    fprintf(stderr, "Child: pipe_fd[0]=%d is CLOSED or invalid\n",
            pipe_fd[0]);
  if (fcntl(pipe_fd[1], F_GETFD) != -1)
    fprintf(stderr, "Child: pipe_fd[1]=%d is OPEN before assignment\n",
            pipe_fd[1]);
  else
    fprintf(stderr, "Child: pipe_fd[1]=%d is CLOSED or invalid\n",
            pipe_fd[1]);
  fprintf(stderr, "\n");
}
/*---------------------------------------------------------------------------*/
int iter_pipe_fork_exec(int n_pipe, DynArray_T oTokens,
                        int is_background) {
  char *args[MAX_ARGS_CNT];
  int pipe_fd[2];    // 0: read, 1: write
  int save_fd = -1;  // save stdout of prev cmd
  int pgid = 0;      // pgid of the first process
  int cmd_start = 0; // start index of cmd
  int cmd_end = 0;   // end index of cmd
  pid_t pid;

  sigset_t mask_all, mask_prev;
  sigfillset(&mask_all);
  sigprocmask(SIG_BLOCK, &mask_all, &mask_prev);

  // create job
  int job_id = 0;
  if (is_background) { // 1: background
    job_id = add_job(background);
  } else { // 0: foreground
    job_id = add_job(foreground);
  }

  // get job
  struct job *job = find_job_by_jid(job_id);
  job->total_num = n_pipe + 1;
  job->curr_num = job->total_num;
  job->pid_list = malloc(sizeof(pid_t) * (n_pipe + 1));
  if (job->pid_list == NULL) {
    perror("malloc failed");
    exit(EXIT_FAILURE);
  }

  // loop each cmd
  for (int i = 0; i <= n_pipe; i++) {
    // create pipe except for the last cmd
    if (i != n_pipe && pipe(pipe_fd) < 0) {
      perror("pipe failed");
      exit(EXIT_FAILURE);
    }

    // find cmd start and end
    find_pipe_loc(oTokens, &cmd_start, &cmd_end);

    // fork
    pid = fork();
    if (pid < 0) {
      perror("fork failed");
      exit(EXIT_FAILURE);
    }

    if (pid == 0) { // child
      // unblock signals
      sigprocmask(SIG_SETMASK, &mask_prev, NULL);

      // set pgid
      if (setpgid(0, pgid) < 0) {
        perror("setpgid failed");
        exit(EXIT_FAILURE);
      }
      pgid = getpgid(0);

      // input redirection
      if (i != 0) {
        if (save_fd != -1) {
          if (dup2(save_fd, STDIN_FILENO) < 0) { // read from prev cmd
            perror("dup2 failed");
            exit(EXIT_FAILURE);
          }
          // close unused pipe
          close(save_fd);
        }
      }
      // output redirection
      if (i != n_pipe) {
        if (dup2(pipe_fd[1], STDOUT_FILENO) < 0) {
          perror("dup2 failed");
          exit(EXIT_FAILURE);
        }
      }

      // close pipe
      if (i != n_pipe) {
        close(pipe_fd[0]);
        close(pipe_fd[1]);
      }
      // build cmd
      build_command_partial(oTokens, cmd_start, cmd_end, args);

      execvp(args[0], args);
      error_print(args[0], PERROR);
      exit(EXIT_FAILURE);
    } else { // parent
      if (i != 0) {
        if (save_fd != -1) {
          // close previous cmd stdout. need to close to send EOF to
          // current child cmd
          close(save_fd);
        }
      }

      // save stdout of current cmd and close pipe write
      if (i != n_pipe) {
        save_fd = pipe_fd[0];
        close(pipe_fd[1]);
      }

      // update job
      if (i == 0) {
        job->pgid = pid; // set pgid of first process
      }
      // add pid to list and increment counter
      job->pid_list[i] = pid;
    }
  }
  // unblock signals
  sigprocmask(SIG_SETMASK, &mask_prev, NULL);
  if (is_background) {
    print_job(job_id, pid);
  } else {
    wait_fg(job_id);
  }

  free(job->pid_list);
  return job_id;
}
/*---------------------------------------------------------------------------*/