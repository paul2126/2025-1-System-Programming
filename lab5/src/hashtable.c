/*---------------------------------------------------------------------------*/
/* hashtable.c */
/* Author: Junghan Yoon, KyoungSoo Park */
/* Modified by: Ryu MyungHyun */
/*---------------------------------------------------------------------------*/
#include "hashtable.h"
/*---------------------------------------------------------------------------*/
int hash(const char *key, size_t hash_size) {
  TRACE_PRINT();
  unsigned int hash = 0;
  while (*key) {
    hash = (hash << 5) + *key++;
  }

  return hash % hash_size;
}
/*---------------------------------------------------------------------------*/
hashtable_t *hash_init(size_t hash_size, int delay) {
  TRACE_PRINT();
  int i, j, ret;
  hashtable_t *table = malloc(sizeof(hashtable_t));

  if (table == NULL) {
    DEBUG_PRINT("Failed to allocate memory for hash table");
    return NULL;
  }

  table->hash_size = hash_size;

  table->buckets = malloc(hash_size * sizeof(node_t *));
  if (table->buckets == NULL) {
    DEBUG_PRINT("Failed to allocate memory for hash table buckets");
    free(table);
    return NULL;
  }

  table->locks = malloc(hash_size * sizeof(rwlock_t));
  if (table->locks == NULL) {
    DEBUG_PRINT("Failed to allocate memory for hash table locks");
    free(table->buckets);
    free(table);
    return NULL;
  }

  table->bucket_sizes =
      malloc(hash_size * sizeof(*table->bucket_sizes));
  if (table->bucket_sizes == NULL) {
    DEBUG_PRINT(
        "Failed to allocate memory for hash table bucket sizes");
    free(table->buckets);
    free(table->locks);
    free(table);
    return NULL;
  }

  for (i = 0; i < hash_size; i++) {
    table->buckets[i] = NULL;
    table->bucket_sizes[i] = 0;
    ret = rwlock_init(&table->locks[i], delay);
    if (ret != 0) {
      DEBUG_PRINT("Failed to initialize read-write lock");
      for (j = 0; j < i; j++) {
        rwlock_destroy(&table->locks[j]);
      }
      free(table->buckets);
      free(table->locks);
      free(table->bucket_sizes);
      free(table);
      return NULL;
    }
  }

  return table;
}
/*---------------------------------------------------------------------------*/
int hash_destroy(hashtable_t *table) {
  TRACE_PRINT();
  node_t *node, *tmp;
  int i;

  for (i = 0; i < table->hash_size; i++) {
    node = table->buckets[i];
    while (node) {
      tmp = node;
      node = node->next;
      free(tmp->key);
      free(tmp->value);
      free(tmp);
    }
    if (rwlock_destroy(&table->locks[i]) != 0) {
      DEBUG_PRINT("Failed to destroy read-write lock");
      return -1;
    }
  }

  free(table->buckets);
  free(table->locks);
  free(table->bucket_sizes);
  free(table);

  return 0;
}
/*---------------------------------------------------------------------------*/
int hash_insert(hashtable_t *table, const char *key,
                const char *value) {
  TRACE_PRINT();
  node_t *node;
  rwlock_t *lock;
  unsigned int index = hash(key, table->hash_size);

  /*---------------------------------------------------------------------------*/
  /* edit here */
  // try lock
  lock = &table->locks[index];
  if (rwlock_write_lock(lock) != 0) {
    DEBUG_PRINT("fail get write lock");
    return -1; // error locking
  }

  // check key exist
  node = table->buckets[index];
  while (node) {
    printf("Checking node with key: %s and %s\n", node->key, key);
    if (strcmp(node->key, key) == 0) {
      // key already exists
      rwlock_write_unlock(lock);
      return 0; // collision
    }
    node = node->next;
  }
  // printf("Inserting key: %s, value: %s into index %u\n", key, value,
  //        index);
  // printf("Current bucket size: %zu\n", table->bucket_sizes[index]);

  // allocate new node
  node = malloc(sizeof(node_t));
  node->key = strdup(key);
  node->key_size = strlen(key);
  node->value = strdup(value);
  node->value_size = strlen(value);
  node->next = table->buckets[index];

  table->buckets[index] = node;
  table->bucket_sizes[index]++;

  // node_t *n = table->buckets[index];
  // while (n) {
  //   printf(" - Key: %s, Value: %s\n", n->key, n->value);
  //   n = n->next;
  // }

  if (rwlock_write_unlock(lock) != 0) {
    DEBUG_PRINT("fail free write lock");
    free(node->key);
    free(node->value);
    free(node);
    return -1; // error releasing lock
  }

  /*---------------------------------------------------------------------------*/

  /* inserted */
  return 1;
}
/*---------------------------------------------------------------------------*/
int hash_search(hashtable_t *table, const char *key, char *dst) {
  TRACE_PRINT();
  node_t *node;
  rwlock_t *lock;
  unsigned int index = hash(key, table->hash_size);

  /*---------------------------------------------------------------------------*/
  /* edit here */
  // try lock
  lock = &table->locks[index];
  if (rwlock_read_lock(lock) != 0) {
    DEBUG_PRINT("fail get read lock");
    return -1; // error locking
  }

  // check key exist
  node = table->buckets[index];
  while (node) {
    if (strcmp(node->key, key) == 0) {
      // key exists
      if (dst) {
        // copy value to dst
        strncpy(dst, node->value, node->value_size);
        dst[node->value_size] = '\0'; // ensure null termination

        // release lock
        if (rwlock_read_unlock(lock) != 0) {
          DEBUG_PRINT("fail free read lock");
          free(node->key);
          free(node->value);
          free(node);
          return -1; // error releasing lock
        }
      }
      return 1; // found
    }
    node = node->next;
  }

  /* !caveat! if found, copy to dst before release read lock */

  /*---------------------------------------------------------------------------*/

  /* key not found */
  return 0;
}
/*---------------------------------------------------------------------------*/
int hash_update(hashtable_t *table, const char *key,
                const char *value) {
  TRACE_PRINT();
  node_t *node;
  rwlock_t *lock;
  char *new_value;
  unsigned int index = hash(key, table->hash_size);

  /*---------------------------------------------------------------------------*/
  /* edit here */
  // try lock
  lock = &table->locks[index];
  if (rwlock_write_lock(lock) != 0) {
    DEBUG_PRINT("fail get write lock");
    return -1; // error locking
  }

  // check key exist
  node = table->buckets[index];
  while (node) {
    if (strcmp(node->key, key) == 0) {
      printf("key found");
      // key exists
      new_value = strdup(value);
      node->value_size = strlen(new_value);
      node->value = new_value; // update value

      // release lock
      if (rwlock_write_unlock(lock) != 0) {
        DEBUG_PRINT("fail free write lock");
        free(node->key);
        free(node->value);
        free(node);
        return -1; // error releasing lock
      }
      return 1; // updated
    }
    node = node->next;
  }

  /*---------------------------------------------------------------------------*/

  /* key not found */
  return 0;
}
/*---------------------------------------------------------------------------*/
int hash_delete(hashtable_t *table, const char *key) {
  TRACE_PRINT();
  node_t *node, *prev = NULL;
  rwlock_t *lock;
  unsigned int index = hash(key, table->hash_size);

  /*---------------------------------------------------------------------------*/
  /* edit here */
  // try lock
  lock = &table->locks[index];
  if (rwlock_write_lock(lock) != 0) {
    DEBUG_PRINT("fail get write lock");
    return -1; // error locking
  }

  // check key exist
  node = table->buckets[index];
  while (node) {
    if (strcmp(node->key, key) == 0) {
      // key exists
      node_t *tmp = node;

      if (prev) {
        prev->next = node->next; // skip deleted node
      } else {
        table->buckets[index] = node->next; // delete head node
      }

      node = node->next; // move to next node
      free(tmp->key);
      free(tmp->value);
      free(tmp);

      table->bucket_sizes[index]--;

      // release lock
      if (rwlock_write_unlock(lock) != 0) {
        DEBUG_PRINT("fail free write lock");
        free(node->key);
        free(node->value);
        free(node);
        return -1; // error releasing lock
      }
      return 1; // deleted
    }
    prev = node;
    node = node->next;
  }

  /*---------------------------------------------------------------------------*/

  /* key not found */
  return 0;
}
/*---------------------------------------------------------------------------*/
/* function to dump the contents of the hash table, including locks
 * status */
void hash_dump(hashtable_t *table) {
  TRACE_PRINT();
  node_t *node;
  int i;
  size_t total_entries = 0;

  printf("[Hash Table Dump]");
  for (i = 0; i < table->hash_size; i++) {
    total_entries += table->bucket_sizes[i];
  }
  printf("Total Entries: %ld\n", total_entries);

  for (i = 0; i < table->hash_size; i++) {
    if (!table->bucket_sizes[i]) {
      continue;
    }
    printf("Bucket %d: %ld entries\n", i, table->bucket_sizes[i]);
    printf("  Lock State -> Read Count: %d, Write Count: %d\n",
           table->locks[i].read_count, table->locks[i].write_count);
    node = table->buckets[i];
    while (node) {
      printf("    Key:   %s\n"
             "    Value: %s\n",
             node->key, node->value);
      node = node->next;
    }
  }
  printf("End of Dump\n");
}