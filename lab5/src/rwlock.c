/*---------------------------------------------------------------------------*/
/* rwlock.c */
/* Author: Junghan Yoon, KyoungSoo Park */
/* Modified by: Ryu MyungHyun */
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
  if (pthread_mutex_lock(&rw->lock) != 0) {
    return -1; // fail to lock mutext
  }
  // printf("read lock\n%ld\n", pthread_self());
  rw->read_count++; // make read_count also count for pending
  while (rw->write_count > 0) {
    if (pthread_cond_wait(&rw->readers, &rw->lock) != 0) {
      // attempt to unlock before returning
      pthread_mutex_unlock(&rw->lock);
      return -1;
    }
  }
  if (pthread_mutex_unlock(&rw->lock) != 0) {
    return -1; // fail to unlock mutext
  }

  /*---------------------------------------------------------------------------*/
  return 0;
}
/*---------------------------------------------------------------------------*/
int rwlock_read_unlock(rwlock_t *rw) {
  sleep(rw->delay); // do not remove this line
  TRACE_PRINT();
  /*---------------------------------------------------------------------------*/
  /* edit here */
  if (pthread_mutex_lock(&rw->lock) != 0) {
    return -1; // fail to lock mutext
  }
  rw->read_count--;
  // printf("read unlock\n%ld\n", pthread_self());

  if (rw->read_count == 0 &&
      rw->writer_ring_head != rw->writer_ring_tail) {
    if (pthread_cond_broadcast(&rw->writers) != 0) {
      pthread_mutex_unlock(&rw->lock);
      return -1; // fail to broadcast condition variable
    }
  }

  if (pthread_mutex_unlock(&rw->lock) != 0) {
    return -1; // fail to unlock mutext
  }

  /*---------------------------------------------------------------------------*/
  return 0;
}
/*---------------------------------------------------------------------------*/
int rwlock_write_lock(rwlock_t *rw) {
  TRACE_PRINT();
  /*---------------------------------------------------------------------------*/
  /* edit here */
  if (pthread_mutex_lock(&rw->lock) != 0) {
    return -1; // fail to lock mutext
  }
  // add to writer ring
  rw->writer_ring[rw->writer_ring_head] = pthread_self();
  rw->writer_ring_head = (rw->writer_ring_head + 1) % WRITER_RING_SIZE;
  // printf("write lock trying\n");
  // printf("%ld: writer ring head: %d, tail: %d\n", pthread_self(),
  //        rw->writer_ring_head, rw->writer_ring_tail);
  // printf("%ld: readcount: %d, writecount: %d\n", pthread_self(),
  //        rw->read_count, rw->write_count);
  while (rw->read_count > 0 || rw->write_count > 0 ||
         !pthread_equal(rw->writer_ring[rw->writer_ring_tail],
                        pthread_self())) {
    // checking if there is reader or writer or if the current thread is
    // not the last then wait
    if (pthread_cond_wait(&rw->writers, &rw->lock) != 0) {
      // attempt to unlock before returning
      pthread_mutex_unlock(&rw->lock);
      return -1;
    }
  }
  // printf("write lock\n");
  // printf("%ld: writer ring head: %d, tail: %d\n", pthread_self(),
  //        rw->writer_ring_head, rw->writer_ring_tail);
  // printf("%ld: readcount: %d, writecount: %d\n", pthread_self(),
  //        rw->read_count, rw->write_count);
  rw->write_count++;
  // deque from writer ring
  rw->writer_ring_tail = (rw->writer_ring_tail + 1) % WRITER_RING_SIZE;
  if (pthread_mutex_unlock(&rw->lock) != 0) {
    return -1; // fail to unlock mutext
  }
  /*---------------------------------------------------------------------------*/
  return 0;
}
/*---------------------------------------------------------------------------*/
int rwlock_write_unlock(rwlock_t *rw) {
  sleep(rw->delay); // do not remove this line
  TRACE_PRINT();
  /*---------------------------------------------------------------------------*/
  /* edit here */
  if (pthread_mutex_lock(&rw->lock) != 0) {
    return -1; // fail to lock mutext
  }
  rw->write_count--;
  // printf("write unlock\n%ld\n", pthread_self());
  if (rw->write_count == 0) {
    // first give priority to readers
    if (pthread_cond_broadcast(&rw->readers) != 0) {
      pthread_mutex_unlock(&rw->lock);
      return -1; // fail to broadcast condition variable
    }

    // if there is writer waiting
    if (rw->writer_ring_head != rw->writer_ring_tail) {
      // then wake up the next writer
      if (pthread_cond_broadcast(&rw->writers) != 0) {
        pthread_mutex_unlock(&rw->lock);
        return -1; // fail to signal condition variable
      }
    }
  }

  if (pthread_mutex_unlock(&rw->lock) != 0) {
    return -1; // fail to unlock mutext
  }
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