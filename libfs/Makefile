CC := gcc
CFLAGS := -Wall -Wextra -Werror
lib := libfs.a
objs := disk.o fs.o

all: $(lib)

deps := $(patsubset %.o, %.d, $(objs))
-include $(deps)

$(lib): $(objs)
	ar rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(lib) $(objs) $(deps)