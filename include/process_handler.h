#ifndef PROCESS_HANDLER_H
#define PROCESS_HANDLER_H

#include <stddef.h>
#include <sys/types.h>

#define DEFAULT_CHUNK_SIZE 4096

enum io { READ, WRITE };

enum layout { LAYOUT_CYCLIC, LAYOUT_BLOCK };

static inline int owner_of_row(int i, int num_procs, int layout, int n) {
  if(layout == LAYOUT_CYCLIC) {
    return i % num_procs;
  }
  int per = (n + num_procs - 1) / num_procs;
  return i / per;
}

static inline int owns_row(int i, int id, int num_procs, int layout, int n) {
  return owner_of_row(i, num_procs, layout, n) == id;
}

typedef struct {
  int* to_child;
  int* to_parent;
  int num_procs;
  int chunk_size;
  int layout;
  int is_parent;
  int my_id;
  int my_read;
  int my_write;
} Process_Pool;

Process_Pool* generate_process_pool(int num_procs, int chunk_size, int layout);

void reap_process_pool(Process_Pool* pool);
void free_process_pool(Process_Pool* pool);

int chwrite(int fd, const char* buf, size_t count, int chunk_size);
int chread (int fd,       char* buf, size_t count, int chunk_size);

#endif
