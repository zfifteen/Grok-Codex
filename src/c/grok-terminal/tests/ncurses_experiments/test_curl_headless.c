/*
 * Headless test: CURL streaming with ncurses
 * Tests if ncurses refreshes block or interfere with CURL streaming
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
static int bytes_received = 0;
static int refresh_method = 0;  // 0 = wrefresh, 1 = wnoutrefresh+doupdate

/* CURL write callback */
size_t streaming_callback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t total_size = size * nmemb;
    chunks_received++;
    bytes_received += total_size;

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

int main() {
    CURL *curl;
    CURLcode res;

    /* Redirect ncurses to /dev/null */
    FILE *term_output = fopen("/dev/null", "w");
    if (!term_output) {
        fprintf(stderr, "Failed to open /dev/null\n");
        return 1;
    }

    SCREEN *screen = newterm(NULL, term_output, stdin);
    if (!screen) {
        fprintf(stderr, "Failed to initialize ncurses screen\n");
        fclose(term_output);
        return 1;
    }

    set_term(screen);
    cbreak();
    noecho();

    /* Create streaming output window */
    int height = LINES;
    int width = COLS;
    streaming_window = newwin(height, width, 0, 0);
    scrollok(streaming_window, TRUE);

    /* Initialize CURL */
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (!curl) {
        endwin();
        delscreen(screen);
        fclose(term_output);
        fprintf(stderr, "Failed to initialize CURL\n");
        return 1;
    }

    /* Test URL that streams data */
    const char *test_url = "https://httpbin.org/stream/100";

    /* Test 1: Using wrefresh() */
    refresh_method = 0;
    chunks_received = 0;
    bytes_received = 0;
    wclear(streaming_window);

    struct timeval start1, end1;
    gettimeofday(&start1, NULL);

    curl_easy_setopt(curl, CURLOPT_URL, test_url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, streaming_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, NULL);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    res = curl_easy_perform(curl);
    gettimeofday(&end1, NULL);

    long