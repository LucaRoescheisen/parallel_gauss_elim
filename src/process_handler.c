#include "../include/process_handler.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/socket.h>

int chwrite(int fd, const char* buf, size_t count, int chunk_size) {
  size_t done = 0;

  while(done < count) {
    size_t want = count - done;
    if(want > (size_t)chunk_size) { 
      want = chunk_size;
    }

    ssize_t n = write(fd, buf + done, want);

    if(n < 0) {
      perror("chwrite"); 
      return -1;}
    done += (size_t)n;
  }

  return 0;
}

int chread(int fd, char* buf, size_t count, int chunk_size) {
  size_t done = 0;

  while(done < count) {
    size_t want = count - done;
    if(want > (size_t)chunk_size) { 
      want = chunk_size; 
    }

    ssize_t n = read(fd, buf + done, want);

    if(n < 0) { 
      perror("chread"); return -1;
    }
    if(n == 0) {
      fprintf(stderr, "chread: unexpected EOF after %zu of %zu bytes\n", done, count);
      return -1;
    }
    done += (size_t)n;
  }

  return 0;
}

Process_Pool* generate_process_pool(int num_procs, int chunk_size, int layout, int ipc_type) {
  if(num_procs <= 0) { 
    return nullptr; 
  }

  Process_Pool* pool = malloc(sizeof(Process_Pool));
  pool->to_child = malloc(num_procs * sizeof(int));
  pool->to_parent = malloc(num_procs * sizeof(int));

  pool->num_procs = num_procs;
  pool->chunk_size = chunk_size;
  pool->layout = layout;
  pool->ipc_type = ipc_type;
  pool->is_parent = 1;
  pool->my_id = 0;
  pool->my_read = -1;
  pool->my_write = -1;

  for(int w = 1; w < num_procs; w++) {
    int down[2], up[2];

    if(ipc_type == SOCKETPAIR) {
      int sv[2];

      if(socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == -1) {
        perror("socketpair");
        free_process_pool(pool);
        return nullptr;
      }

      down[WRITE] = sv[0];
      up[READ] = sv[0];
      down[READ] = sv[1];
      up[WRITE] = sv[1];
    }
    else if(pipe(down) == -1 || pipe(up) == -1) {
      perror("pipe");
      free_process_pool(pool);
      return nullptr;
    }

    pid_t pid = fork();

    if(pid == -1) {
      perror("fork");
      free_process_pool(pool);
      return nullptr;
    }
    else if(pid == 0) {

      pool->is_parent = 0;
      pool->my_id = w;
      pool->my_read = down[READ];
      pool->my_write = up[WRITE];

      close(down[WRITE]);

      if(ipc_type == PIPE) {
        close(up[READ]);
      }

      for(int p = 1; p < w; p++) {
        close(pool->to_child[p]);

        if(pool->to_parent[p] != pool->to_child[p]) {
          close(pool->to_parent[p]);
        }
      }

      return pool;
    }
    else {

      pool->to_child[w] = down[WRITE];
      pool->to_parent[w] = up[READ];

      close(down[READ]);

      if(ipc_type == PIPE) {
        close(up[WRITE]);
      }
    }
  }

  return pool;
}

void reap_process_pool(Process_Pool* pool) {
  for(int w = 1; w < pool->num_procs; w++) {
    close(pool->to_child[w]);

    if(pool->to_parent[w] != pool->to_child[w]) {
      close(pool->to_parent[w]);
    }
  }
  for(int w = 1; w < pool->num_procs; w++) {
    wait(nullptr);
  }
}

void free_process_pool(Process_Pool* pool) {
  free(pool->to_child);
  free(pool->to_parent);
  free(pool);
}
