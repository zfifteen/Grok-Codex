/*
 * Test 1: Compare wrefresh() vs wnoutrefresh()+doupdate()
 *
 * This test measures the performance difference between:
 * - wrefresh(win): Immediate refresh of single window
 * - wnoutrefresh(win) + doupdate(): Batched refresh of multiple windows
 *
 * Expected: wnoutrefresh()+doupdate() should be faster for multiple updates
 */

#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#define NUM_ITERATIONS 1000
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
        wrefresh(win1);  // Immediate refresh

        if (i % 10 == 0) {
            wprintw(win2, "Status: %d\n", i);
            wrefresh(win2);  // Immediate refresh
        }
    }

    return get_time_us() - start;
}

/* Test 2: Using wnoutrefresh() + doupdate() */
long long test_wnoutrefresh_doupdate(WINDOW *win1, WINDOW *win2) {
    long long start = get_time_us();

    for (int i = 0; i < NUM_ITERATIONS; i++) {
        wprintw(win1, TEST_STRING, i);
        wnoutrefresh(win1);  // Mark for refresh, don't update yet

        if (i % 10 == 0) {
            wprintw(win2, "Status: %d\n", i);
            wnoutrefresh(win2);  // Mark for refresh
        }

        doupdate();  // Batch update all marked windows
    }

    return get_time_us() - start;
}

int main() {
    /* Initialize ncurses */
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    /* Create two windows (simulating left and right panes) */
    int height = LINES;
    int width = COLS / 2;
    WINDOW *left_pane = newwin(height, width, 0, 0);
    WINDOW *right_pane = newwin(height, width, 0, width);

    scrollok(left_pane, TRUE);
    scrollok(right_pane, TRUE);

    /* Display test info */
    mvprintw(0, 0, "Testing ncurses refresh methods...");
    mvprintw(1, 0, "Iterations: %d", NUM_ITERATIONS);
    mvprintw(2, 0, "Press any key to start Test 1 (wrefresh)...");
    refresh();
    getch();

    /* Clear windows for test */
    wclear(left_pane);
    wclear(right_pane);

    /* Test 1: wrefresh() */
    long long time1 = test_wrefresh(left_pane, right_pane);

    /* Show results */
    clear();
    mvprintw(0, 0, "Test 1 (wrefresh) completed: %lld microseconds (%.2f ms)",
             time1, time1 / 1000.0);
    mvprintw(1, 0, "Press any key to start Test 2 (wnoutrefresh+doupdate)...");
    refresh();
    getch();

    /* Clear windows for test 2 */
    wclear(left_pane);
    wclear(right_pane);

    /* Test 2: wnoutrefresh() + doupdate() */
    long long time2 = test_wnoutrefresh_doupdate(left_pane, right_pane);

    /* Show final results */
    clear();
    mvprintw(0, 0, "=== RESULTS ===");
    mvprintw(2, 0, "Test 1 (wrefresh):             %lld us (%.2f ms)",
             time1, time1 / 1000.0);
    mvprintw(3, 0, "Test 2 (wnoutrefresh+doupdate): %lld us (%.2f ms)",
             time2, time2 / 1000.0);
    mvprintw(5, 0, "Difference: %lld us (%.2f ms)",
             time1 - time2, (time1 - time2) / 1000.0);
    mvprintw(6, 0, "Speedup: %.2fx", (double)time1 / time2);

    if (time2 < time1) {
        mvprintw(8, 0, "WINNER: wnoutrefresh+doupdate is %.2f%% faster",
                 ((double)(time1 - time2) / time1) * 100);
    } else {
        mvprintw(8, 0, "WINNER: wrefresh is %.2f%% faster",
                 ((double)(time2 - time1) / time2) * 100);
    }

    mvprintw(10, 0, "Press any key to exit...");
    refresh();
    getch();

    /* Cleanup */
    delwin(left_pane);
    delwin(right_pane);
    endwin();

    return 0;
}