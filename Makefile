CC = gcc
CFLAGS = -std=c2x -Wall -Wextra -g -O3 -march=native -MMD -MP -fsanitize=address
LDLIBS = -lm
AR = ar
ARFLAGS = rcs

SRC = $(filter-out src/main.c src/fdm_fork.c src/pgaussElim.c src/thread_handler.c \
                   src/pgaussElimFork.c src/process_handler.c, $(wildcard src/*.c))
OBJ = $(patsubst src/%.c, obj/%.o, $(SRC))
DEP = $(wildcard obj/*.d)

LIBOBJ = obj/pgaussElim.o obj/thread_handler.o obj/pgaussElimFork.o obj/process_handler.o
LIB = obj/libgauss.a

all: assignment fdm_fork

assignment: obj/main.o $(OBJ) $(LIB)
	$(CC) $(CFLAGS) -o $@ obj/main.o $(OBJ) -Lobj -lgauss $(LDLIBS)

fdm_fork: obj/fdm_fork.o $(OBJ) $(LIB)
	$(CC) $(CFLAGS) -o $@ obj/fdm_fork.o $(OBJ) -Lobj -lgauss $(LDLIBS)

$(LIB): $(LIBOBJ)
	$(AR) $(ARFLAGS) $@ $^

obj/%.o: src/%.c
	@mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@

-include $(DEP)

clean:
	rm -rf obj assignment fdm_fork

.PHONY: all clean
