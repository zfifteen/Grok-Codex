/*
 * Test 2: ncurses with CURL streaming callback
 *
 * This test simulates the actual grok-terminal scenario:
 * - CURL streaming callback that receives chunks of data
 * - Data needs to be displayed in ncurses window in real-time
 * - Tests if ncurses updates block CURL or cause corruption
 *
 * Expected: wnoutrefresh()+doupdate() should work without blocking
 */

#include <ncurses.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

/* Global window for callback access */
static WINDOW *streaming_window = NULL;
static int chunks_received = 0;
static int refresh_method = 0;  // 0 = wrefresh, 1 = wnoutrefresh+doupdate

/* CURL write callback - simulates grok-terminal's write_callback */
size_t streaming_callback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t total_size = size * nmemb;
    chunks_received++;

    if (!streaming_window) {
        return total_size;
    }

    /* Write chunk to ncurses window */
    char buffer[1024];
    size_t to_copy = (total_size < sizeof(buffer) - 1) ? total_size : sizeof(buffer) - 1;
    memcpy(buffer, ptr, to_copy);
    buffer[to_copy] = '\0';

    /* Display in window */
    wprintw(streaming_window, "%s", buffer);

    /* Refresh using selected method */
    if (refresh_method == 0) {
        wrefresh(streaming_window);
    } else {
        wnoutrefresh(streaming_window);
        doupdate();
    }

    return total_size;
}

int main(int argc, char *argv[]) {
    CURL *curl;
    CURLcode res;

    /* Initialize ncurses */
    initscr();
    cbreak();
    noecho();
    nodelay(stdscr, TRUE);  // Non-blocking input

    /* Create streaming output window */
    int height = LINES - 5;
    int width = COLS;
    streaming_window = newwin(height, width, 0, 0);
    scrollok(streaming_window, TRUE);

    /* Status window */
    WINDOW *status_win = newwin(5, width, height, 0);
    box(status_win, 0, 0);

    /* Initialize CURL */
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (!curl) {
        endwin();
        fprintf(stderr, "Failed to initialize CURL\n");
        return 1;
    }

    /* Test URLs that stream data */
    const char *test_url = "https://httpbin.org/stream/100";  // Streams 100 JSON objects

    mvwprintw(status_win, 1, 2, "Testing CURL streaming with ncurses...");
    mvwprintw(status_win, 2, 2, "URL: %s", test_url);
    wrefresh(status_win);

    /* Test 1: Using wrefresh() */
    refresh_method = 0;
    chunks_received = 0;
    wclear(streaming_window);
    mvwprintw(status_win, 3, 2, "Test 1: Using wrefresh() - Press any key to start");
    wrefresh(status_win);
    getch();

    struct timeval start1, end1;
    gettimeofday(&start1, NULL);

    curl_easy_setopt(curl, CURLOPT_URL, test_url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, streaming_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, NULL);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    res = curl_easy_perform(curl);
    gettimeofday(&end1, NULL);

    long long time1_ms = (end1.tv_sec - start1.tv_sec) * 1000 +
                         (end1.tv_usec - start1.tv_usec) / 1000;
    int chunks1 = chunks_received;

    if (res != CURLE_OK) {
        mvwprintw(status_win, 3, 2, "Test 1 FAILED: %s", curl_easy_strerror(res));
    } else {
        mvwprintw(status_win, 3, 2, "Test 1 completed: %d chunks in %lld ms", chunks1, time1_ms);
    }
    wrefresh(status_win);
    napms(2000);

    /* Test 2: Using wnoutrefresh() + doupdate() */
    refresh_method = 1;
    chunks_received = 0;
    wclear(streaming_window);
    mvwprintw(status_win, 4, 2, "Test 2: Using wnoutrefresh+doupdate - Starting...");
    wrefresh(status_win);
    napms(1000);

    struct timeval start2, end2;
    gettimeofday(&start2, NULL);

    curl_easy_setopt(curl, CURLOPT_URL, test_url);
    res = curl_easy_perform(curl);
    gettimeofday(&end2, NULL);

    long long time2_ms = (end2.tv_sec - start2.tv_sec) * 1000 +
                         (end2.tv_usec - start2.tv_usec) / 1000;
    int chunks2 = chunks_received;

    if (res != CURLE_OK) {
        mvwprintw(status_win, 4, 2, "Test 2 FAILED: %s                                    ",
                  curl_easy_strerror(res));
    } else {
        mvwprintw(status_win, 4, 2, "Test 2 completed: %d chunks in %lld ms                ",
                  chunks2, time2_ms);
    }
    wrefresh(status_win);

    /* Show results */
    napms(2000);
    clear();
    mvprintw(0, 0, "=== CURL STREAMING WITH NCURSES RESULTS ===");
    mvprintw(2, 0, "Test 1 (wrefresh):              %lld ms, %d chunks", time1_ms, chunks1);
    mvprintw(3, 0, "Test 2 (wnoutrefresh+doupdate): %lld ms, %d chunks", time2_ms, chunks2);
    mvprintw(5, 0, "Difference: %lld ms", time1_ms - time2_ms);

    if (time2_ms < time1_ms) {
        mvprintw(6, 0, "WINNER: wnoutrefresh+doupdate is %.2f%% faster",
                 ((double)(time1_ms - time2_ms) / time1_ms) * 100);
    } else if (time1_ms < time2_ms) {
        mvprintw(6, 0, "WINNER: wrefresh is %.2f%% faster",
                 ((double)(time2_ms - time1_ms) / time2_ms) * 100);
    } else {
        mvprintw(6, 0, "RESULT: Both methods performed equally");
    }

    mvprintw(8, 0, "Key finding: %s",
             (res == CURLE_OK) ? "✓ CURL streaming works with ncurses!" :
                                  "✗ CURL streaming had issues");

    mvprintw(10, 0, "Press any key to exit...");
    refresh();
    getch();

    /* Cleanup */
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    delwin(streaming_window);
    delwin(status_win);
    endwin();

    return 0;
}