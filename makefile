CC = gcc
CFLAGS = -g -Wextra -Wall -pedantic -Werror
LIBS = 
LDFLAGS = -g
OBJ = get_data.o put_data.o mlb_sha1.o chunky.o
%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

chunky: $(OBJ)
	gcc -o $@ $^ $(LDFLAGS) $(LIBS)

.PHONY: clean
clean:
	rm -rf *.o chunky
