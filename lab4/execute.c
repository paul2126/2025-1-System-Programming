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
int iter_pipe_fork_exec(int n_pipe, DynArray_T oTokens,
                        int is_background) {
  //
  // TODO-START: iter_pipe_fork_exec() in execute.c
  //
  // If you want to run the newely forked process in the foreground,
  // call wait_fg(). If you want to run the newely forked process
  // in the background, call print_job().
  // All the exited processes should be handled in
  // the sigchld_handler() in snush.c.
  //

  int jobid = 1;
  return jobid;

  //
  // TODO-END: iter_pipe_fork_exec() in execute.c
  //
}
/*---------------------------------------------------------------------------*/