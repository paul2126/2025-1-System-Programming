/*---------------------------------------------------------------------------*/
/* job.c */
/* Author: Jongki Park, Kyoungsoo Park */
/*---------------------------------------------------------------------------*/

#include "job.h"

extern struct job_manager *manager;
/*---------------------------------------------------------------------------*/
void init_job_manager() {
  manager = (struct job_manager *)calloc(1, sizeof(struct job_manager));
  if (manager == NULL) {
    fprintf(stderr, "[Error] job manager allocation failed\n");
    exit(EXIT_FAILURE);
  }
}
/*---------------------------------------------------------------------------*/
struct job *find_job_by_jid(int job_id) {
  if (manager == NULL) {
    fprintf(stderr, "[Error] find_job_by_jid: Job manager is NULL\n");
    return NULL;
  }

  struct job *current = manager->jobs;

  while (current != NULL) {
    if (current->job_id == job_id) {
      return current;
    }
    current = current->next;
  }

  return NULL;
}
/*---------------------------------------------------------------------------*/
struct job *find_job_fg() {
  if (manager == NULL) {
    fprintf(stderr, "[Error] find_job_fg: Job manager is NULL\n");
    return NULL;
  }

  struct job *current = manager->jobs;

  while (current != NULL) {
    if (current->state == foreground) {
      return current;
    }
    current = current->next;
  }

  return NULL;
}
/*---------------------------------------------------------------------------*/
// create and add job to the manager
// return job id
int add_job(char *args, job_state state) {
  // create job
  struct job *new_job = (struct job *)malloc(sizeof(struct job));
  new_job->job_id = manager->n_jobs + 1;
  new_job->state = state;
  new_job->pgid = unknown;
  new_job->pid_list = NULL;
  new_job->total_num = 0;
  new_job->curr_num = 0;
  new_job->next = NULL;

  // assign job to manager
  if (manager->jobs == NULL) {
    manager->jobs = new_job;
  } else { // append to the end
    struct job *current = manager->jobs;
    while (current->next != NULL) {
      current = current->next;
    }
    current->next = new_job;
  }
  return new_job->job_id;
}
/*---------------------------------------------------------------------------*/