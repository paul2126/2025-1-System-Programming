/*---------------------------------------------------------------------------*/
/* rwlock.c */
/* Author: Junghan Yoon, KyoungSoo Park */
/* Modified by: (Your Name) */
/*---------------------------------------------------------------------------*/
#include "rwlock.h"
/*---------------------------------------------------------------------------*/
int rwlock_init(rwlock_t *rw, int delay) {
  TRACE_PRINT();
  int ret, destroy_ret;
  rw->read_count = 0;
  rw->write_count = 0;
  rw->writer_ring_head = 0;
  rw->writer_ring_tail = 0;
  rw->delay = delay;
  if (rw->writer_ring) {
    free(rw->writer_ring);
  }
  rw->writer_ring = calloc(WRITER_RING_SIZE, sizeof(pthread_t));
  if (!rw->writer_ring) {
    return -1;
  }

  ret = pthread_mutex_init(&rw->lock, NULL);
  if (ret != 0) {
    errno = ret;
    return -1;
  }

  ret = pthread_cond_init(&rw->readers, NULL);
  if (ret != 0) {
    /* mutex destroy failed */
    destroy_ret = pthread_mutex_destroy(&rw->lock);
    if (destroy_ret != 0) {
      errno = destroy_ret;
    } else {
      /* original error from cond init */
      errno = ret;
    }
    return -1;
  }

  ret = pthread_cond_init(&rw->writers, NULL);
  if (ret != 0) {
    /* condition variable destroy failed */
    destroy_ret = pthread_cond_destroy(&rw->readers);
    if (destroy_ret != 0) {
      errno = destroy_ret;
    } else {
      /* mutex destroy failed */
      destroy_ret = pthread_mutex_destroy(&rw->lock);
      if (destroy_ret != 0) {
        errno = destroy_ret;
      } else {
        /* original error from cond init */
        errno = ret;
      }
    }
    return -1;
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
int rwlock_read_lock(rwlock_t *rw) {
  TRACE_PRINT();
  /*---------------------------------------------------------------------------*/
  /* edit here */

  /*---------------------------------------------------------------------------*/
  return 0;
}
/*---------------------------------------------------------------------------*/
int rwlock_read_unlock(rwlock_t *rw) {
  sleep(rw->delay); // do not remove this line
  TRACE_PRINT();
  /*---------------------------------------------------------------------------*/
  /* edit here */

  /*---------------------------------------------------------------------------*/
  return 0;
}
/*---------------------------------------------------------------------------*/
int rwlock_write_lock(rwlock_t *rw) {
  TRACE_PRINT();
  /*---------------------------------------------------------------------------*/
  /* edit here */

  /*---------------------------------------------------------------------------*/
  return 0;
}
/*---------------------------------------------------------------------------*/
int rwlock_write_unlock(rwlock_t *rw) {
  sleep(rw->delay); // do not remove this line
  TRACE_PRINT();
  /*---------------------------------------------------------------------------*/
  /* edit here */

  /*---------------------------------------------------------------------------*/
  return 0;
}
/*---------------------------------------------------------------------------*/
int rwlock_destroy(rwlock_t *rw) {
  TRACE_PRINT();
  int ret, unlock_ret;

  /* ensure all locks are released before destroying */
  ret = pthread_mutex_lock(&rw->lock);
  if (ret != 0) {
    errno = ret;
    return -1;
  }

  /* check if there are any active readers or writers */
  if (rw->read_count > 0 || rw->write_count > 0) {
    ret = pthread_mutex_unlock(&rw->lock);
    if (ret != 0) {
      errno = ret;
      return -1;
    }
    fprintf(stderr,
            "Error: rwlock_destroy called while locks are held.\n");
    /* indicate resource is busy */
    errno = EBUSY;
    return -1;
  }

  /* wake up any pending threads */
  if (rw->writer_ring_head != rw->writer_ring_tail) {
    ret = pthread_cond_broadcast(&rw->writers);
    if (ret != 0) {
      /* mutex unlock failed */
      unlock_ret = pthread_mutex_unlock(&rw->lock);
      if (unlock_ret != 0) {
        errno = unlock_ret;
      } else {
        errno = ret;
      }
      return -1;
    }
  }

  /* clean up resources */
  ret = pthread_mutex_unlock(&rw->lock);
  if (ret != 0) {
    errno = ret;
    return -1;
  }

  ret = pthread_mutex_destroy(&rw->lock);
  if (ret != 0) {
    errno = ret;
    return -1;
  }

  ret = pthread_cond_destroy(&rw->readers);
  if (ret != 0) {
    errno = ret;
    return -1;
  }

  ret = pthread_cond_destroy(&rw->writers);
  if (ret != 0) {
    errno = ret;
    return -1;
  }

  if (rw->writer_ring) {
    free(rw->writer_ring);
    rw->writer_ring = NULL;
  }

  return 0;
}