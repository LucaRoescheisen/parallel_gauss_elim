CC = gcc
CFLAGS = -std=c2x -Wall -Wextra -g -O3 -march=native -MMD -MP -fsanitize=address
LDLIBS = -lm
AR = ar
ARFLAGS = rcs

SRC = $(filter-out src/pgaussElim.c, $(wildcard src/*.c))
OBJ = $(patsubst src/%.c, obj/%.o, $(SRC))
DEP = $(OBJ:.o=.d)

LIBOBJ = obj/pgaussElim.o
LIB = obj/libgauss.a

TARGET = assignment
all: $(TARGET)

$(TARGET): $(OBJ) $(LIB)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) -Lobj -lgauss $(LDLIBS)

$(LIB): $(LIBOBJ)
	$(AR) $(ARFLAGS) $@ $^

obj/%.o: src/%.c
	@mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@

-include $(DEP)

clean:
	rm -rf obj $(TARGET)

.PHONY: all clean
