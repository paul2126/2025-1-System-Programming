/*---------------------------------------------------------------------------*/
/* snush.c */
/* Author: Jongki Park, Kyoungsoo Park */
/*---------------------------------------------------------------------------*/

#include "snush.h"
#include "dynarray.h"
#include "execute.h"
#include "job.h"
#include "lexsyn.h"
#include "token.h"
#include "util.h"

struct job_manager *manager;
volatile sig_atomic_t error_flag = 0;
/*---------------------------------------------------------------------------*/
static void check_error_flag() {
  if (error_flag == 1) {
    fprintf(stderr, "[Error] SIGCHLD handler waitpid error\n");
    exit(EXIT_FAILURE);
  }

  //
  // TODO: check_error_flag() start
  //
  // A common strategy is to have a global flag of type sig_atomic_t for
  // the signal handler's error checking.
  //

  //
  // TODO: check_error_flag() end
  //
}
/*---------------------------------------------------------------------------*/
static void check_bg_status() {
  struct job *curr = manager->done_bg_jobs;
  // print finish jobs
  while (curr != NULL) {
    fprintf(stdout, "[%d] Process group: %d done\n", curr->job_id,
            curr->pgid);
    fflush(stdout);

    manager->done_bg_jobs = curr->next;
    // remove done jobs
    free(curr->pid_list);
    free(curr);
    // move curr to next
    curr = manager->done_bg_jobs;
  }
}
/*---------------------------------------------------------------------------*/
static void terminate_jobs() {
  struct job *curr = manager->jobs;
  struct job *next = NULL;
  pid_t pgid;

  // implement kill//////////////////////////////////////////////////
  while (curr != NULL) {
    pgid = curr->pgid;
    if (pgid != 0) {
      kill(-pgid, SIGKILL);
    }
    manager->n_jobs--;
    next = curr->next;
    free(curr->pid_list);
    free(curr);
    curr = next;
  }

  manager->jobs = NULL;

  curr = manager->done_bg_jobs;
  while (curr != NULL) {
    next = curr->next;
    free(curr->pid_list);
    free(curr);
    curr = next;
  }

  manager->done_bg_jobs = NULL;
}
/*---------------------------------------------------------------------------*/
void cleanup() {
  terminate_jobs();
  //
  // TODO: cleanup() start
  //

  //
  // TODO: cleanup() end
  //
  free(manager);
}
/*---------------------------------------------------------------------------*/
static void sigint_handler(int signo) {
  struct job *job = NULL;
  pid_t pgid;

  if (signo == SIGINT) {
    job = find_job_fg();
    if (job == NULL) {
      return;
    }

    pgid = job->pgid;
    if (pgid != 0) {
      kill(-pgid, SIGINT);
    }
  }

  return;
}
/*---------------------------------------------------------------------------*/
/*
 * Whenever a child process terminates, this handler handles all
 * terminated child processes (i.e. zombies). Do not use printf() in
 * signal handler since this is called asynchronously. Instead, print
 * out the error message or the background process status message with
 * check_error_flag() and check_bg_status().
 */
static void sigchld_handler(int signo) {

  pid_t pid;
  int stat;

  if (signo == SIGCHLD) {
    // get all terminated children
    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
      int found_job = 0; // for breaking out of middle while
      struct job *job = NULL;
      struct job *cur_job = manager->jobs;
      while (!found_job && cur_job != NULL) { // loop jobs
        for (int i = 0; i < cur_job->total_num; i++) {
          if (cur_job->pid_list[i] == pid) { // found job
            job = cur_job;
            found_job = 1;
            break;
          }
        }
        cur_job = cur_job->next;
      }
      if (WIFEXITED(stat) || WIFSIGNALED(stat)) {
        if (job != NULL) {
          job->curr_num--;
          if (job->curr_num == 0) {
            // need to remove job to prevent infinite loop
            if (job->state == background) {
              manager->done_bg_jobs = job;
            }
            job->state = stopped;
            manager->jobs = job->next;
            job = job->next;
            manager->n_jobs--;
          }
        }
      }
    }

    if (pid < 0 && errno != ECHILD && errno != EINTR) {
      error_flag = 1;
    }
  }

  return;
}
/*---------------------------------------------------------------------------*/
static void shell_helper(const char *in_line) {
  DynArray_T oTokens;

  enum LexResult lexcheck;
  enum SyntaxResult syncheck;
  enum BuiltinType btype;
  int n_pipe;
  int ret_jid;
  int is_background;

  oTokens = dynarray_new(0);
  if (oTokens == NULL) {
    error_print("Cannot allocate memory", FPRINTF);
    exit(EXIT_FAILURE);
  }

  lexcheck = lex_line(in_line, oTokens);
  switch (lexcheck) {
  case LEX_SUCCESS:
    if (dynarray_get_length(oTokens) == 0) {
      dynarray_map(oTokens, free_token, NULL);
      dynarray_free(oTokens);
      return;
    }

    /* dump lex result when DEBUG is set */
    dump_lex(oTokens);

    syncheck = syntax_check(oTokens);
    if (syncheck == SYN_SUCCESS) {
      btype = check_builtin(dynarray_get(oTokens, 0));
      if (btype == NORMAL) {
        is_background = check_bg(oTokens);
        n_pipe = count_pipe(oTokens);

        if (manager->n_jobs + 1 > MAX_JOBS) {
          fprintf(
              stderr,
              "[Error] Total number of jobs execeed the limit(%d)\n",
              MAX_JOBS);
          return;
        }

        if (n_pipe > 0) {
          ret_jid = iter_pipe_fork_exec(n_pipe, oTokens, is_background);
        } else {
          ret_jid = fork_exec(oTokens, is_background);
        }

        if (ret_jid < 0) {
          error_print("Invalid return value "
                      "of external command execution",
                      FPRINTF);
        }
      } else {
        /* Execute builtin command */
        execute_builtin(oTokens, btype);
      }
    }

    /* syntax error cases */
    else if (syncheck == SYN_FAIL_NOCMD)
      error_print("Missing command name", FPRINTF);
    else if (syncheck == SYN_FAIL_MULTREDOUT)
      error_print("Multiple redirection of standard out", FPRINTF);
    else if (syncheck == SYN_FAIL_NODESTOUT)
      error_print("Standard output redirection without file name",
                  FPRINTF);
    else if (syncheck == SYN_FAIL_MULTREDIN)
      error_print("Multiple redirection of standard input", FPRINTF);
    else if (syncheck == SYN_FAIL_NODESTIN)
      error_print("Standard input redirection without file name",
                  FPRINTF);
    else if (syncheck == SYN_FAIL_INVALIDBG)
      error_print("Invalid use of background", FPRINTF);
    break;

  case LEX_QERROR:
    error_print("Unmatched quote", FPRINTF);
    break;

  case LEX_NOMEM:
    error_print("Cannot allocate memory", FPRINTF);
    break;

  case LEX_LONG:
    error_print("Command is too large", FPRINTF);
    break;

  default:
    error_print("lex_line needs to be fixed", FPRINTF);
    exit(EXIT_FAILURE);
  }

  /* Free memories allocated to tokens */
  dynarray_map(oTokens, free_token, NULL);
  dynarray_free(oTokens);
}
/*---------------------------------------------------------------------------*/
int main(int argc, char *argv[]) {
  sigset_t sigset;
  char c_line[MAX_LINE_SIZE + 2];

  atexit(cleanup);
  init_job_manager();
  error_print(argv[0], SETUP);

  sigemptyset(&sigset);
  sigaddset(&sigset, SIGINT);
  sigaddset(&sigset, SIGCHLD);
  sigprocmask(SIG_UNBLOCK, &sigset, NULL);

  /* Register signal handler */
  signal(SIGINT, sigint_handler);
  signal(SIGCHLD, sigchld_handler);
  signal(SIGQUIT, SIG_IGN);
  signal(SIGPIPE, SIG_IGN);
  signal(SIGTSTP, SIG_IGN);

  while (1) {
    check_error_flag();
    check_bg_status();

    fprintf(stdout, "%% ");
    fflush(stdout);

    if (fgets(c_line, MAX_LINE_SIZE, stdin) == NULL) {
      printf("\n");
      exit(EXIT_SUCCESS);
    }
    shell_helper(c_line);
  }

  return 0;
}
/*---------------------------------------------------------------------------*/