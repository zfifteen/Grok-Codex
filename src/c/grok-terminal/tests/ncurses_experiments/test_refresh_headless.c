/*
 * Headless test: Compare wrefresh() vs wnoutrefresh()+doupdate()
 * No interactive UI - just outputs benchmark results to stdout
 */

#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#define NUM_ITERATIONS 10000
#define TEST_STRING "Streaming output line %d...\n"

/* Get current time in microseconds */
long long get_time_us() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000000 + tv.tv_usec;
}

/* Test 1: Using wrefresh() for each update */
long long test_wrefresh(WINDOW *win1, WINDOW *win2) {
    long long start = get_time_us();

    for (int i = 0; i < NUM_ITERATIONS; i++) {
        wprintw(win1, TEST_STRING, i);
        wrefresh(win1);

        if (i % 10 == 0) {
            wprintw(win2, "Status: %d\n", i);
            wrefresh(win2);
        }
    }

    return get_time_us() - start;
}

/* Test 2: Using wnoutrefresh() + doupdate() */
long long test_wnoutrefresh_doupdate(WINDOW *win1, WINDOW *win2) {
    long long start = get_time_us();

    for (int i = 0; i < NUM_ITERATIONS; i++) {
        wprintw(win1, TEST_STRING, i);
        wnoutrefresh(win1);

        if (i % 10 == 0) {
            wprintw(win2, "Status: %d\n", i);
            wnoutrefresh(win2);
        }

        doupdate();
    }

    return get_time_us() - start;
}

int main() {
    /* Redirect ncurses to /dev/null so we can write results to stdout */
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

    /* Create two windows */
    int height = LINES;
    int width = COLS / 2;
    WINDOW *left_pane = newwin(height, width, 0, 0);
    WINDOW *right_pane = newwin(height, width, 0, width);

    scrollok(left_pane, TRUE);
    scrollok(right_pane, TRUE);

    /* Test 1: wrefresh() */
    wclear(left_pane);
    wclear(right_pane);
    long long time1 = test_wrefresh(left_pane, right_pane);

    /* Test 2: wnoutrefresh() + doupdate() */
    wclear(left_pane);
    wclear(right_pane);
    long long time2 = test_wnoutrefresh_doupdate(left_pane, right_pane);

    /* Cleanup ncurses before printing results */
    delwin(left_pane);
    delwin(right_pane);
    endwin();
    delscreen(screen);
    fclose(term_output);

    /* Now output results to stdout */
    printf("========================================\n");
    printf("NCURSES REFRESH METHOD BENCHMARK\n");
    printf("========================================\n");
    printf("Iterations: %d\n", NUM_ITERATIONS);
    printf("\n");
    printf("Test 1 (wrefresh):              %lld us (%.2f ms)\n", time1, time1 / 1000.0);
    printf("Test 2 (wnoutrefresh+doupdate): %lld us (%.2f ms)\n", time2, time2 / 1000.0);
    printf("\n");
    printf("Difference: %lld us (%.2f ms)\n", labs(time1 - time2), labs(time1 - time2) / 1000.0);

    if (time2 < time1) {
        printf("Speedup:    %.2fx\n", (double)time1 / time2);
        printf("\n");
        printf("✓ WINNER: wnoutrefresh+doupdate is %.2f%% faster\n",
               ((double)(time1 - time2) / time1) * 100);
    } else if (time1 < time2) {
        printf("Speedup:    %.2fx\n", (double)time2 / time1);
        printf("\n");
        printf("✓ WINNER: wrefresh is %.2f%% faster\n",
               ((double)(time2 - time1) / time2) * 100);
    } else {
        printf("\n");
        printf("= RESULT: Both methods performed equally\n");
    }

    printf("========================================\n");

    return 0;
}