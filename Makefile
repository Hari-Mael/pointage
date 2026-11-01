CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 -fstack-protector-strong -D_FORTIFY_SOURCE=2
TARGET := pointage

$(TARGET): main.c
	$(CC) $(CFLAGS) -o $@ main.c

test: $(TARGET)
	./$(TARGET) --selftest

clean:
	rm -f $(TARGET)

.PHONY: test clean
