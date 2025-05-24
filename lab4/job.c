/*---------------------------------------------------------------------------*/
/* job.c                                                                     */
/* Author: Jongki Park, Kyoungsoo Park                                       */
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
//
// TODO: new job control functions in job.c start
// 

//
// TODO: new job control functions in job.c end
// 
/*---------------------------------------------------------------------------*/