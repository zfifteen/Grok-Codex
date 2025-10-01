# Makefile for ncurses experiments

CC = cc
CFLAGS = -Wall -Wextra -O2 -std=c11
NCURSES_LIBS = $(shell pkg-config --libs ncurses 2>/dev/null || echo "-lncurses")
CURL_LIBS = $(shell pkg-config --libs libcurl 2>/dev/null || echo "-lcurl")

TARGETS = test_refresh_methods test_curl_streaming test_refresh_headless

.PHONY: all clean run-all

all: $(TARGETS)

test_refresh_methods: test_refresh_methods.c
	$(CC) $(CFLAGS) -o $@ $< $(NCURSES_LIBS)

test_curl_streaming: test_curl_streaming.c
	$(CC) $(CFLAGS) -o $@ $< $(NCURSES_LIBS) $(CURL_LIBS)

test_refresh_headless: test_refresh_headless.c
	$(CC) $(CFLAGS) -o $@ $< $(NCURSES_LIBS)

run-all: all
	@echo "=== Running Test 1: Refresh Methods ==="
	@echo "This test will compare wrefresh() vs wnoutrefresh()+doupdate()"
	@echo ""
	./test_refresh_methods
	@echo ""
	@echo "=== Running Test 2: CURL Streaming ==="
	@echo "This test simulates actual grok-terminal streaming scenario"
	@echo ""
	./test_curl_streaming

clean:
	rm -f $(TARGETS)
	rm -f *.o
