/*---------------------------------------------------------------------------*/
/* job.h */
/* Author: Jongki Park, Kyoungsoo Park */
/*---------------------------------------------------------------------------*/

#ifndef _JOB_H_
#define _JOB_H_

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_JOBS 16

typedef enum State {
  unknown = 0,
  foreground,
  background,
  stopped,
} job_state;

/*
 * Job = The user's command line input
 * ex) if the user's command line input is "ps -ef | grep job" then
 * One job, Two processes.
 */
struct job {
  int job_id;
  job_state state;
  pid_t pgid;
  pid_t *pid_list;
  int total_num;
  int curr_num;
  struct job *next;
};

/*
 * One global variable for a job manager.
 * When a job is created, register it with the job manager,
 * regardless of whether it is a foreground or background job.
 * When the background job is finished, add it in the done_bg_jobs.
 */
struct job_manager {
  int n_jobs;
  struct job *jobs;
  struct job *done_bg_jobs;
};

void init_job_manager();
struct job *find_job_by_jid(int job_id);
struct job *find_job_fg();

//
// TODO: new job control functions in job.h start
//

int add_job(job_state state);
//
// TODO: new job control functions in job.h end
//

#endif /* _JOB_H_ */