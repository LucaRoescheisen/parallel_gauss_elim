CC = gcc
CFLAGS = -std=c2x -Wall -Wextra -g -O3 -march=native -MMD -MP
SRC = $(wildcard src/*.c)
OBJ = $(patsubst src/%.c, obj/%.o, $(SRC))
DEP = $(OBJ:.o=d)
TARGET = assignment

all: $(TARGET)

$(TARGET) : $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

obj/%.o: src/%.c
	@mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@

-include $(DEP)

clean:
	rm -rf obj $(TARGET)

.PHONY: all clean
