// cc main.c $(pkg-config --libs --cflags raylib) -lm -o main
// ./main

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include "raylib.h"

// Configuration

#define WIDTH      1200
#define HEIGHT      720
#define COUNT        30
#define BAR_GAP       2
#define FPS_DEFAULT  30
#define STEPS_DEFAULT 1

// Colors

#define COL_BG        (Color){15,  15,  20,  255}
#define COL_BG_TOP    (Color){12,  18,  32,  255}
#define COL_BG_BOT    (Color){6,   8,   14,  255}
#define COL_BAR       (Color){90,  90, 110,  255}
#define COL_BAR_EDGE  (Color){170, 180, 210, 140}

#define COL_COMPARE   (Color){255, 200,  50,  255}  // yellow  – comparing
#define COL_SWAP      (Color){220,  60,  60,  255}  // red     – swapping
#define COL_SORTED    (Color){60,  200, 100,  255}  // green   – done
#define COL_PIVOT     (Color){80,  160, 255,  255}  // blue    – pivot

#define COL_TEXT      (Color){220, 220, 230,  255}
#define COL_DIM       (Color){120, 120, 140,  255}
#define COL_PANEL     (Color){22,  22,  30,  255}
#define COL_CARD      (Color){20,  28,  44,  225}
#define COL_CARD_ALT  (Color){16,  22,  34,  215}
#define COL_ACCENT    (Color){98,  208, 255, 255}

// Sorting algorithms

typedef enum {
    SORT_BUBBLE = 0,
    SORT_SELECTION,
    SORT_INSERTION,
    SORT_MERGE,
    SORT_QUICK,
    SORT_HEAP,
    SORT_COUNT
} SortType;

const char *SORT_NAMES[SORT_COUNT] = {
    "Bubble Sort",
    "Selection Sort",
    "Insertion Sort",
    "Merge Sort",
    "Quick Sort",
    "Heap Sort"
};

// Complexity strings
const char *SORT_TIME[SORT_COUNT]  = { "O(n²)", "O(n²)", "O(n²)", "O(n log n)", "O(n log n)", "O(n log n)" };
const char *SORT_BEST[SORT_COUNT]  = { "O(n)",  "O(n²)", "O(n)",  "O(n log n)", "O(n log n)", "O(n log n)" };
const char *SORT_SPACE[SORT_COUNT] = { "O(1)",  "O(1)",  "O(1)",  "O(n)",       "O(log n)",   "O(1)"       };

const char *SORT_DESC[SORT_COUNT] = {
    "Compares adjacent elements and\nmoves the largest one to the end\nwith each pass. Simple but slow.",
    "Finds the minimum in the unsorted\npart and places it at the front.\nAlways O(n^2) comparisons.",
    "Builds a sorted prefix by inserting\neach element into its correct\nposition, like sorting playing cards.",
    "Divides the array in half, sorts\nboth halves recursively, then merges\nthem. Guaranteed O(n log n).",
    "Selects a pivot, separates smaller\nelements left and larger right,\nthen recurses on both sides.",
    "Builds a max-heap, then repeatedly\nextracts the maximum to sort\nin place. Always O(n log n)."
};

// Bar states for coloring

typedef enum {
    BAR_NORMAL = 0,
    BAR_COMPARE,
    BAR_SWAP,
    BAR_SORTED,
    BAR_PIVOT
} BarState;

// Global state

int        numbers[COUNT];
float      display_numbers[COUNT];
BarState   bar_states[COUNT];
bool       sorted_mask[COUNT];   // permanently green once confirmed sorted

int        comparisons = 0;
int        swaps_count = 0;
bool       sort_done   = false;
SortType   current_sort = SORT_BUBBLE;

// Utility functions

void swap(int i, int j) {
    int tmp = numbers[i];
    numbers[i] = numbers[j];
    numbers[j] = tmp;
    swaps_count++;
}

void init_numbers(void) {
    for (int i = 0; i < COUNT; i++) numbers[i] = i + 1;
    for (int i = COUNT - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int tmp = numbers[i]; numbers[i] = numbers[j]; numbers[j] = tmp;
    }
    for (int i = 0; i < COUNT; i++) {
        display_numbers[i] = (float)numbers[i];
        bar_states[i] = BAR_NORMAL;
        sorted_mask[i] = false;
    }
    comparisons = 0; swaps_count = 0; sort_done = false;
}

// Bubble Sort

SortType bubble_reset_flag = -1;
static int bs_i = 0, bs_j = 0;
static bool bs_swapped_pass = false;

void bubble_reset(void) { bs_i = 0; bs_j = 0; bs_swapped_pass = false; }

bool bubble_step(void) {
    // clear previous highlights
    for (int k = 0; k < COUNT; k++) if (!sorted_mask[k]) bar_states[k] = BAR_NORMAL;

    if (bs_i >= COUNT - 1) { // all done
        for (int k = 0; k < COUNT; k++) { bar_states[k] = BAR_SORTED; sorted_mask[k] = true; }
        return true;
    }

    if (bs_j >= COUNT - 1 - bs_i) {
        sorted_mask[COUNT - 1 - bs_i] = true;
        bar_states[COUNT - 1 - bs_i] = BAR_SORTED;
        if (!bs_swapped_pass) { // already sorted
            for (int k = 0; k < COUNT; k++) { bar_states[k] = BAR_SORTED; sorted_mask[k] = true; }
            return true;
        }
        bs_i++; bs_j = 0; bs_swapped_pass = false;
        return false;
    }

    comparisons++;
    bar_states[bs_j] = BAR_COMPARE;
    bar_states[bs_j + 1] = BAR_COMPARE;

    if (numbers[bs_j] > numbers[bs_j + 1]) {
        swap(bs_j, bs_j + 1);
        bar_states[bs_j] = BAR_SWAP;
        bar_states[bs_j + 1] = BAR_SWAP;
        bs_swapped_pass = true;
    }
    bs_j++;
    return false;
}

// Selection Sort

static int sel_i = 0, sel_min = 0, sel_j = 0;

void selection_reset(void) { sel_i = 0; sel_min = 0; sel_j = 1; }

bool selection_step(void) {
    for (int k = 0; k < COUNT; k++) if (!sorted_mask[k]) bar_states[k] = BAR_NORMAL;

    if (sel_i >= COUNT - 1) {
        sorted_mask[COUNT-1] = true; bar_states[COUNT-1] = BAR_SORTED;
        for (int k = 0; k < COUNT; k++) { bar_states[k] = BAR_SORTED; sorted_mask[k] = true; }
        return true;
    }

    if (sel_j >= COUNT) {
        // place minimum
        if (sel_min != sel_i) { swap(sel_i, sel_min); bar_states[sel_i] = BAR_SWAP; }
        sorted_mask[sel_i] = true; bar_states[sel_i] = BAR_SORTED;
        sel_i++; sel_min = sel_i; sel_j = sel_i + 1;
        return false;
    }

    comparisons++;
    bar_states[sel_min] = BAR_PIVOT;
    bar_states[sel_j]   = BAR_COMPARE;

    if (numbers[sel_j] < numbers[sel_min]) sel_min = sel_j;
    sel_j++;
    return false;
}

// Insertion Sort 

static int ins_i = 1, ins_j = 1;
static int ins_key = 0;
static bool ins_shifting = false;

void insertion_reset(void) { ins_i = 1; ins_j = 1; ins_key = 0; ins_shifting = false; }

bool insertion_step(void) {
    for (int k = 0; k < COUNT; k++) if (!sorted_mask[k]) bar_states[k] = BAR_NORMAL;

    if (ins_i >= COUNT) {
        for (int k = 0; k < COUNT; k++) { bar_states[k] = BAR_SORTED; sorted_mask[k] = true; }
        return true;
    }

    if (!ins_shifting) {
        ins_key = numbers[ins_i];
        ins_j = ins_i;
        ins_shifting = true;
    }

    if (ins_j > 0 && numbers[ins_j - 1] > ins_key) {
        comparisons++;
        numbers[ins_j] = numbers[ins_j - 1];
        bar_states[ins_j]     = BAR_SWAP;
        bar_states[ins_j - 1] = BAR_COMPARE;
        ins_j--;
        swaps_count++;
    } else {
        numbers[ins_j] = ins_key;
        bar_states[ins_j] = BAR_SORTED;
        ins_i++;
        ins_shifting = false;

        for (int k = 0; k < ins_i; k++) sorted_mask[k] = true;
    }
    return false;
}

// Merge Sort

typedef struct { int l, m, r, k, i, j; bool active; } MergeState;
static MergeState ms = {0};
static int  merge_width = 1;
static int  merge_left  = 0;
static int  merge_buf[COUNT];
static bool merge_copying = false;
static int  merge_copy_k = 0;

void merge_reset(void) {
    merge_width = 1; merge_left = 0; merge_copying = false;
    ms.active = false;
}

bool merge_step(void) {
    for (int k = 0; k < COUNT; k++) if (!sorted_mask[k]) bar_states[k] = BAR_NORMAL;

    if (merge_width >= COUNT) {
        for (int k = 0; k < COUNT; k++) { bar_states[k] = BAR_SORTED; sorted_mask[k] = true; }
        return true;
    }

    if (!ms.active) {
        if (merge_left >= COUNT) { merge_width *= 2; merge_left = 0; return false; }
        ms.l = merge_left;
        ms.m = merge_left + merge_width - 1;
        if (ms.m >= COUNT) { merge_left = COUNT; return false; }
        ms.r = (merge_left + 2 * merge_width - 1 < COUNT - 1) ? merge_left + 2 * merge_width - 1 : COUNT - 1;
        ms.i = ms.l; ms.j = ms.m + 1; ms.k = ms.l;
        ms.active = true; merge_copying = false;
        for (int x = ms.l; x <= ms.r; x++) merge_buf[x] = numbers[x];
        merge_left += 2 * merge_width;
        return false;
    }

    if (!merge_copying) {
        if (ms.i <= ms.m && ms.j <= ms.r) {
            comparisons++;
            bar_states[ms.i] = BAR_COMPARE;
            bar_states[ms.j] = BAR_COMPARE;
            if (merge_buf[ms.i] <= merge_buf[ms.j]) { numbers[ms.k] = merge_buf[ms.i++]; }
            else                                     { numbers[ms.k] = merge_buf[ms.j++]; swaps_count++; }
            bar_states[ms.k] = BAR_SWAP;
            ms.k++;
        } else {
            while (ms.i <= ms.m) { numbers[ms.k++] = merge_buf[ms.i++]; }
            while (ms.j <= ms.r) { numbers[ms.k++] = merge_buf[ms.j++]; }
            ms.active = false;
            if (merge_width * 2 >= COUNT) {
                for (int x = 0; x < COUNT; x++) { bar_states[x] = BAR_SORTED; sorted_mask[x] = true; }
                return true;
            }
        }
    }
    return false;
}

// Quick Sort

#define QS_STACK 64
static int qs_stack_lo[QS_STACK], qs_stack_hi[QS_STACK], qs_top = -1;
static int qs_lo, qs_hi, qs_pivot_val, qs_i, qs_j;
static bool qs_partitioning = false;

void quick_reset(void) {
    qs_top = 0; qs_stack_lo[0] = 0; qs_stack_hi[0] = COUNT - 1;
    qs_partitioning = false;
}

bool quick_step(void) {
    for (int k = 0; k < COUNT; k++) if (!sorted_mask[k]) bar_states[k] = BAR_NORMAL;

    if (!qs_partitioning) {
        if (qs_top < 0) {
            for (int k = 0; k < COUNT; k++) { bar_states[k] = BAR_SORTED; sorted_mask[k] = true; }
            return true;
        }
        qs_lo = qs_stack_lo[qs_top];
        qs_hi = qs_stack_hi[qs_top--];
        if (qs_lo >= qs_hi) return false;
        qs_pivot_val = numbers[qs_hi];
        qs_i = qs_lo - 1;
        qs_j = qs_lo;
        qs_partitioning = true;
    }

    bar_states[qs_hi] = BAR_PIVOT;

    if (qs_j < qs_hi) {
        comparisons++;
        bar_states[qs_j] = BAR_COMPARE;
        if (numbers[qs_j] <= qs_pivot_val) {
            qs_i++;
            swap(qs_i, qs_j);
            bar_states[qs_i] = BAR_SWAP;
            bar_states[qs_j] = BAR_SWAP;
        }
        qs_j++;
    } else {
        // place pivot
        swap(qs_i + 1, qs_hi);
        int pivot_idx = qs_i + 1;
        bar_states[pivot_idx] = BAR_SORTED;
        sorted_mask[pivot_idx] = true;

        if (pivot_idx - 1 > qs_lo) { qs_stack_lo[++qs_top] = qs_lo;          qs_stack_hi[qs_top] = pivot_idx - 1; }
        if (pivot_idx + 1 < qs_hi) { qs_stack_lo[++qs_top] = pivot_idx + 1;  qs_stack_hi[qs_top] = qs_hi; }
        qs_partitioning = false;
    }
    return false;
}

// Heap Sort

static int heap_n, heap_i, heap_phase;
// p0: building heap (i down), p1: extraction

static int heap_heapify_root, heap_heapify_n, heap_heapify_k;
static bool heap_heapifying = false;

void heap_reset(void) {
    heap_n = COUNT; heap_phase = 0;
    heap_i = COUNT / 2 - 1;
    heap_heapifying = false;
}

bool heap_step(void) {
    for (int k = 0; k < COUNT; k++) if (!sorted_mask[k]) bar_states[k] = BAR_NORMAL;

    if (heap_heapifying) {
        int largest = heap_heapify_root;
        int l = 2 * heap_heapify_root + 1;
        int r = 2 * heap_heapify_root + 2;
        comparisons++;
        if (l < heap_heapify_n && numbers[l] > numbers[largest]) largest = l;
        if (r < heap_heapify_n && numbers[r] > numbers[largest]) largest = r;
        bar_states[heap_heapify_root] = BAR_PIVOT;
        if (largest != heap_heapify_root) {
            swap(heap_heapify_root, largest);
            bar_states[largest] = BAR_SWAP;
            heap_heapify_root = largest;
        } else {
            heap_heapifying = false;
        }
        return false;
    }

    if (heap_phase == 0) {
        if (heap_i < 0) { heap_phase = 1; heap_i = heap_n - 1; return false; }
        heap_heapify_root = heap_i--;
        heap_heapify_n    = heap_n;
        heap_heapifying   = true;
        return false;
    }

    // p1: extract max
    if (heap_i <= 0) {
        sorted_mask[0] = true; bar_states[0] = BAR_SORTED;
        for (int k = 0; k < COUNT; k++) { bar_states[k] = BAR_SORTED; sorted_mask[k] = true; }
        return true;
    }
    swap(0, heap_i);
    sorted_mask[heap_i] = true; bar_states[heap_i] = BAR_SORTED;
    heap_i--;
    heap_heapify_root = 0;
    heap_heapify_n    = heap_i + 1;
    heap_heapifying   = true;
    return false;
}

// Sort dispatch

void reset_sort(void) {
    for (int i = 0; i < COUNT; i++) { bar_states[i] = BAR_NORMAL; sorted_mask[i] = false; }
    comparisons = 0; swaps_count = 0; sort_done = false;
    switch (current_sort) {
        case SORT_BUBBLE:    bubble_reset();    break;
        case SORT_SELECTION: selection_reset(); break;
        case SORT_INSERTION: insertion_reset(); break;
        case SORT_MERGE:     merge_reset();     break;
        case SORT_QUICK:     quick_reset();     break;
        case SORT_HEAP:      heap_reset();      break;
        default: break;
    }
}

bool do_sort_step(void) {
    switch (current_sort) {
        case SORT_BUBBLE:    return bubble_step();
        case SORT_SELECTION: return selection_step();
        case SORT_INSERTION: return insertion_step();
        case SORT_MERGE:     return merge_step();
        case SORT_QUICK:     return quick_step();
        case SORT_HEAP:      return heap_step();
        default: return true;
    }
}

// Drawing

#define PANEL_W   260
#define BAR_AREA_W (WIDTH - PANEL_W)
#define BAR_TOP   60
#define BAR_BOT   (HEIGHT - 30)
#define BAR_H     (BAR_BOT - BAR_TOP)

int count_sorted_items(void) {
    int done = 0;
    for (int i = 0; i < COUNT; i++) {
        if (sorted_mask[i]) done++;
    }
    return done;
}

Color bar_color(int idx) {
    if (sorted_mask[idx]) return COL_SORTED;
    switch (bar_states[idx]) {
        case BAR_COMPARE: return COL_COMPARE;
        case BAR_SWAP:    return COL_SWAP;
        case BAR_PIVOT:   return COL_PIVOT;
        default:          return COL_BAR;
    }
}

void draw_bars(void) {
    DrawRectangleGradientV(0, 0, BAR_AREA_W, HEIGHT, COL_BG_TOP, COL_BG_BOT);

    for (int g = BAR_TOP; g <= BAR_BOT; g += 60) {
        DrawLineEx((Vector2){24, (float)g}, (Vector2){BAR_AREA_W - 24, (float)g}, 1.0f, Fade(COL_DIM, 0.18f));
    }

    DrawRectangleRounded((Rectangle){24, BAR_TOP + 4, BAR_AREA_W - 48, BAR_H + 8}, 0.035f, 16, Fade(WHITE, 0.03f));

    float bar_w = (float)(BAR_AREA_W - BAR_GAP) / COUNT;
    for (int i = 0; i < COUNT; i++) {
        float h = display_numbers[i] / COUNT * BAR_H;
        float x = (float)i * bar_w + BAR_GAP / 2.0f;
        float y = BAR_BOT - h;
        Rectangle shadow = {x + 2, y + 6, bar_w - BAR_GAP, h};
        Rectangle bar = {x, y, bar_w - BAR_GAP, h};
        DrawRectangleRounded(shadow, 0.18f, 10, Fade(BLACK, 0.30f));
        DrawRectangleRounded(bar, 0.18f, 10, bar_color(i));
        DrawRectangleRoundedLinesEx(bar, 0.18f, 10, 1.0f, Fade(COL_BAR_EDGE, 0.75f));

        char value[8];
        snprintf(value, sizeof(value), "%d", numbers[i]);
        Vector2 size = MeasureTextEx(GetFontDefault(), value, 10, 1);
        if (h > 28) {
            DrawTextEx(GetFontDefault(), value,
                       (Vector2){x + (bar.width - size.x) / 2.0f, y + 8},
                       10, 1, Fade(COL_TEXT, 0.85f));
        }
    }
}

void draw_card(int x, int y, int w, int h, Color bg) {
    DrawRectangleRounded((Rectangle){x + 4, y + 8, w, h}, 0.08f, 12, Fade(BLACK, 0.22f));
    DrawRectangleRounded((Rectangle){x, y, w, h}, 0.08f, 12, bg);
    DrawRectangleRoundedLinesEx((Rectangle){x, y, w, h}, 0.08f, 12, 1.0f, Fade(COL_BAR_EDGE, 0.45f));
}

void draw_panel(Font font, int steps_per_second, bool paused) {
    DrawRectangleGradientV(BAR_AREA_W, 0, PANEL_W, HEIGHT, (Color){18, 24, 38, 255}, (Color){10, 12, 20, 255});
    DrawLineEx((Vector2){BAR_AREA_W, 0}, (Vector2){BAR_AREA_W, HEIGHT}, 1, Fade(COL_BAR_EDGE, 0.45f));

    int px = BAR_AREA_W + 16;
    int py = 20;
    int card_w = PANEL_W - 32;
    int sorted_items = count_sorted_items();
    float progress = (float)sorted_items / COUNT;

    draw_card(px, py, card_w, 84, COL_CARD);
    DrawTextEx(font, "Sorting Visualizer", (Vector2){px + 14, py + 12}, 13, 1, COL_ACCENT);
    DrawTextEx(font, SORT_NAMES[current_sort], (Vector2){px + 14, py + 34}, 22, 1, COL_TEXT);
    const char *status_text = sort_done ? "Completed" : (paused ? "Manual Mode" : "Auto Play");
    Color status_color = sort_done ? COL_SORTED : (paused ? COL_COMPARE : COL_ACCENT);
    DrawTextEx(font, status_text, (Vector2){px + 14, py + 60}, 12, 1, status_color);
    py += 100;

    draw_card(px, py, card_w, 96, COL_CARD_ALT);
    DrawTextEx(font, "Progress", (Vector2){px + 14, py + 12}, 13, 1, COL_DIM);
    DrawRectangleRounded((Rectangle){px + 14, py + 36, card_w - 28, 14}, 0.5f, 10, Fade(COL_TEXT, 0.12f));
    DrawRectangleRounded((Rectangle){px + 14, py + 36, (card_w - 28) * progress, 14}, 0.5f, 10, COL_ACCENT);
    char progress_text[64];
    snprintf(progress_text, sizeof(progress_text), "%d / %d placed correctly", sorted_items, COUNT);
    DrawTextEx(font, progress_text, (Vector2){px + 14, py + 58}, 12, 1, COL_TEXT);
    py += 112;

    draw_card(px, py, card_w, 112, COL_CARD);
    DrawTextEx(font, "Complexity", (Vector2){px + 14, py + 12}, 13, 1, COL_DIM);
    char buf[64]; // buffer for formatting complexity strings !!
    snprintf(buf, sizeof(buf), "Avg:   %s", SORT_TIME[current_sort]);
    DrawTextEx(font, buf, (Vector2){px + 14, py + 34}, 13, 1, COL_TEXT);
    snprintf(buf, sizeof(buf), "Best:  %s", SORT_BEST[current_sort]);
    DrawTextEx(font, buf, (Vector2){px + 14, py + 52}, 13, 1, COL_TEXT);
    snprintf(buf, sizeof(buf), "Space: %s", SORT_SPACE[current_sort]);
    DrawTextEx(font, buf, (Vector2){px + 14, py + 70}, 13, 1, COL_TEXT);
    snprintf(buf, sizeof(buf), "Speed: %d steps/s", steps_per_second);
    DrawTextEx(font, buf, (Vector2){px + 14, py + 88}, 13, 1, COL_TEXT);
    py += 128;

    draw_card(px, py, card_w, 128, COL_CARD_ALT);
    DrawTextEx(font, "How it works", (Vector2){px + 14, py + 12}, 13, 1, COL_DIM);
    const char *desc = SORT_DESC[current_sort];
    char line[64]; int li = 0;
    int line_y = py + 34;
    for (int di = 0; ; di++) {
        if (desc[di] == '\n' || desc[di] == '\0') {
            line[li] = '\0';
            DrawTextEx(font, line, (Vector2){px + 14, line_y}, 12, 1, COL_TEXT);
            line_y += 17; li = 0;
            if (desc[di] == '\0') break;
        } else { line[li++] = desc[di]; }
    }
    py += 144;

    draw_card(px, py, card_w, 94, COL_CARD);
    DrawTextEx(font, "Stats", (Vector2){px + 14, py + 12}, 13, 1, COL_DIM);
    snprintf(buf, sizeof(buf), "Comparisons: %d", comparisons);
    DrawTextEx(font, buf, (Vector2){px + 14, py + 34}, 13, 1, COL_TEXT);
    snprintf(buf, sizeof(buf), "Swaps:       %d", swaps_count);
    DrawTextEx(font, buf, (Vector2){px + 14, py + 52}, 13, 1, COL_TEXT);
    snprintf(buf, sizeof(buf), "Array size:  %d", COUNT);
    DrawTextEx(font, buf, (Vector2){px + 14, py + 70}, 13, 1, COL_TEXT);
    py += 110;

    draw_card(px, py, card_w, 114, COL_CARD_ALT);
    DrawTextEx(font, "Legend", (Vector2){px + 14, py + 12}, 13, 1, COL_DIM);

    Color legend_cols[] = { COL_BAR, COL_COMPARE, COL_SWAP, COL_SORTED, COL_PIVOT };
    const char *legend_labels[] = { "Unsorted", "Comparing", "Swapping", "Sorted", "Pivot" };
    for (int li2 = 0; li2 < 5; li2++) {
        DrawRectangleRounded((Rectangle){px + 14, py + 34 + li2 * 16, 12, 12}, 0.3f, 6, legend_cols[li2]);
        DrawTextEx(font, legend_labels[li2], (Vector2){px + 32, py + 32 + li2 * 16}, 12, 1, COL_TEXT);
    }
    py += 130;

    draw_card(px, py, card_w, 100, COL_CARD);
    DrawTextEx(font, "Controls", (Vector2){px + 14, py + 12}, 13, 1, COL_DIM);
    const char *controls[] = {
        "1-6  switch algorithm",
        "Space  auto/manual",
        "Right  next step",
        "R    shuffle & restart",
        "+/-  change auto speed"
    };
    for (int ci = 0; ci < 5; ci++) {
        DrawTextEx(font, controls[ci], (Vector2){px + 14, py + 34 + ci * 16}, 11, 1, COL_DIM);
    }
}

void draw_topbar(Font font) {
    // algorithm selector tabs
    float tab_w = (float)BAR_AREA_W / SORT_COUNT;
    for (int i = 0; i < SORT_COUNT; i++) {
        Color bg = (i == current_sort) ? (Color){24, 50, 78, 255} : (Color){18, 24, 38, 220};
        Color tc = (i == current_sort) ? COL_ACCENT : COL_DIM;
        DrawRectangleRounded((Rectangle){i * tab_w + 2, 8, tab_w - 6, 34}, 0.25f, 10, bg);
        // short names
        const char *short_names[] = {"Bubble", "Selection", "Insertion", "Merge", "Quick", "Heap"};
        char label[32];
        snprintf(label, sizeof(label), "[%d] %s", i + 1, short_names[i]);
        int tw = MeasureTextEx(font, label, 12, 1).x;
        DrawTextEx(font, label, (Vector2){i * tab_w + (tab_w - tw) / 2, 18}, 12, 1, tc);
    }
    DrawLineEx((Vector2){0, 48}, (Vector2){BAR_AREA_W, 48}, 1, Fade(COL_BAR_EDGE, 0.40f));

    DrawTextEx(font,
               "Press 1-6 to change algorithm  |  Space: auto/manual  |  Right Arrow: single step",
               (Vector2){20, 60}, 14, 1, Fade(COL_TEXT, 0.88f));
}

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------// 
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------//

int main(void) {
    srand((unsigned int)time(NULL));
    init_numbers();
    reset_sort();

    InitWindow(WIDTH, HEIGHT, "sorting-visualizer");
    SetTargetFPS(FPS_DEFAULT);

    Font font = GetFontDefault();

    int fps = FPS_DEFAULT;
    int steps_per_second = STEPS_DEFAULT;
    float step_accumulator = 0.0f;
    bool paused = true;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        
        if (IsKeyPressed(KEY_R)) {
            init_numbers();
            reset_sort();
            paused = true;
            step_accumulator = 0.0f;
        }
        if (IsKeyPressed(KEY_SPACE)) paused = !paused;

        if (IsKeyPressed(KEY_NINE) || IsKeyPressed(KEY_KP_ADD)) {
            steps_per_second = (steps_per_second < 240) ? steps_per_second + 5 : steps_per_second;
        }
        if (IsKeyPressed(KEY_EIGHT) || IsKeyPressed(KEY_KP_SUBTRACT)) {
            steps_per_second = (steps_per_second > 5) ? steps_per_second - 5 : steps_per_second;
        }

        for (int k = 0; k < SORT_COUNT; k++) {
            if (IsKeyPressed(KEY_ONE + k)) {
                current_sort = (SortType)k;
                init_numbers();
                reset_sort();
                paused = true;
                step_accumulator = 0.0f;
                break;
            }
        }

        // Steps
        if (!paused && !sort_done) {
            step_accumulator += dt * steps_per_second;
            while (step_accumulator >= 1.0f && !sort_done) {
                sort_done = do_sort_step();
                step_accumulator -= 1.0f;
            }
        } else if (paused) {
            step_accumulator = 0.0f;
        }

        if (paused && !sort_done && (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D))) {
            sort_done = do_sort_step();
        }

        for (int i = 0; i < COUNT; i++) {
            float blend = fminf(1.0f, dt * 12.0f);
            display_numbers[i] += (numbers[i] - display_numbers[i]) * blend;
        }

        // Draw
        BeginDrawing();
        ClearBackground(COL_BG);
        draw_topbar(font);
        draw_bars();
        draw_panel(font, steps_per_second, paused);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
