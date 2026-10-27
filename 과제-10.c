#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define N          10000
#define TRIALS     100
#define MAXKEY     1000000
#define SWAP_COUNT (N / 20)
#define SEED       20240601ULL

typedef struct {
    int key1;
    int key2;
    int order;
} Data;

static long long g_comp = 0;
static long long g_move = 0;

static unsigned long long rng_state = SEED;
static unsigned long long rng_next(void) {
    rng_state ^= rng_state >> 12;
    rng_state ^= rng_state << 25;
    rng_state ^= rng_state >> 27;
    return rng_state * 2685821657736338717ULL;
}
static int rand_range(int lo, int hi) {
    return lo + (int)(rng_next() % (unsigned long long)(hi - lo + 1));
}

static void swap_data(Data *a, Data *b) {
    Data temp = *a; g_move++;
    *a = *b;        g_move++;
    *b = temp;      g_move++;
}

static void selection_sort(Data *a, int n) {
    for (int i = 0; i < n - 1; i++) {
        int min = i;
        for (int j = i + 1; j < n; j++) {
            g_comp++;
            if (a[j].key1 < a[min].key1) min = j;
        }
        if (min != i) swap_data(&a[i], &a[min]);
    }
}

static void bubble_sort(Data *a, int n) {
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - 1 - i; j++) {
            g_comp++;
            if (a[j].key1 > a[j + 1].key1) swap_data(&a[j], &a[j + 1]);
        }
    }
}

static void bubble_sort_early(Data *a, int n) {
    for (int i = 0; i < n - 1; i++) {
        int swapped = 0;
        for (int j = 0; j < n - 1 - i; j++) {
            g_comp++;
            if (a[j].key1 > a[j + 1].key1) {
                swap_data(&a[j], &a[j + 1]);
                swapped = 1;
            }
        }
        if (!swapped) break;
    }
}

static void insertion_sort_gt(Data *a, int n) {
    for (int i = 1; i < n; i++) {
        Data current = a[i]; g_move++;
        int j = i - 1;
        while (j >= 0) {
            g_comp++;
            if (a[j].key1 > current.key1) {
                a[j + 1] = a[j]; g_move++;
                j--;
            } else break;
        }
        a[j + 1] = current; g_move++;
    }
}

static void insertion_sort_ge(Data *a, int n) {
    for (int i = 1; i < n; i++) {
        Data current = a[i]; g_move++;
        int j = i - 1;
        while (j >= 0) {
            g_comp++;
            if (a[j].key1 >= current.key1) {
                a[j + 1] = a[j]; g_move++;
                j--;
            } else break;
        }
        a[j + 1] = current; g_move++;
    }
}

typedef void (*SortFn)(Data *, int);

typedef struct {
    const char *name;
    SortFn fn;
} Algo;

static const Algo ALGOS[] = {
    {"Selection Sort",                selection_sort},
    {"Bubble Sort",                   bubble_sort},
    {"Bubble Sort (Early Termination)", bubble_sort_early},
    {"Insertion Sort",                insertion_sort_gt},
};
#define NALGO ((int)(sizeof(ALGOS) / sizeof(ALGOS[0])))

static int is_sorted(const Data *a, int n) {
    for (int i = 0; i + 1 < n; i++)
        if (a[i].key1 > a[i + 1].key1) return 0;
    return 1;
}

static int is_permutation(const Data *sorted, const Data *orig, int n) {
    char *seen = (char *)calloc((size_t)n, 1);
    int ok = 1;
    for (int i = 0; i < n && ok; i++) {
        int o = sorted[i].order;
        if (o < 0 || o >= n || seen[o] ||
            orig[o].key1 != sorted[i].key1 || orig[o].key2 != sorted[i].key2)
            ok = 0;
        else seen[o] = 1;
    }
    free(seen);
    return ok;
}

static int stability_violations(const Data *a, int n) {
    int v = 0;
    for (int i = 0; i + 1 < n; i++)
        if (a[i].key1 == a[i + 1].key1 && a[i].order > a[i + 1].order) v++;
    return v;
}

static void gen_random(Data *a, int n) {
    for (int i = 0; i < n; i++) {
        a[i].key1  = rand_range(0, MAXKEY);
        a[i].key2  = rand_range(0, MAXKEY);
        a[i].order = i;
    }
}

static int cmp_prep(const void *x, const void *y) {
    const Data *p = (const Data *)x, *q = (const Data *)y;
    if (p->key1 != q->key1) return (p->key1 > q->key1) - (p->key1 < q->key1);
    return (p->order > q->order) - (p->order < q->order);
}

static void run_one(SortFn fn, const Data *src, const Data *ref, Data *work, int n,
                    long long *comp, long long *move, int *ok) {
    memcpy(work, src, sizeof(Data) * (size_t)n);
    g_comp = 0; g_move = 0;
    fn(work, n);
    *comp = g_comp; *move = g_move;
    *ok = is_sorted(work, n) && is_permutation(work, ref, n);
}

static void run_state(const char *title, const Data *src, const Data *ref, Data *work) {
    printf("%s\n\n", title);
    for (int k = 0; k < NALGO; k++) {
        long long c, m; int ok;
        run_one(ALGOS[k].fn, src, ref, work, N, &c, &m, &ok);
        printf("%s\n", ALGOS[k].name);
        printf("Comparisons : %lld\n", c);
        printf("Movements   : %lld\n", m);
        printf("Verified    : %s\n\n", ok ? "OK (sorted)" : "FAIL");
    }
    printf("------------------------------------\n\n");
}

static void print_arr(const char *label, const Data *a, int n) {
    printf("  %-22s:", label);
    for (int i = 0; i < n; i++)
        printf(" (%d,%d)#%d", a[i].key1, a[i].key2, a[i].order);
    printf("\n");
}

static void stability_report(const char *name, SortFn fn,
                             const Data *smalldata, int sn,
                             const Data *big, int bn) {
    Data sbuf[16];
    Data *bbuf = (Data *)malloc(sizeof(Data) * (size_t)bn);

    memcpy(sbuf, smalldata, sizeof(Data) * (size_t)sn);
    fn(sbuf, sn);
    int sv = stability_violations(sbuf, sn);

    memcpy(bbuf, big, sizeof(Data) * (size_t)bn);
    fn(bbuf, bn);
    int bv = stability_violations(bbuf, bn);
    int sorted_ok = is_sorted(bbuf, bn);

    printf("%-20s : %s  (sorted=%s, 4-element violations=%d, 1000-element violations=%d)\n",
           name, (sv == 0 && bv == 0) ? "STABLE" : "UNSTABLE",
           sorted_ok ? "OK" : "FAIL", sv, bv);
    print_arr("4-element result", sbuf, sn);
    free(bbuf);
}

int main(void) {
    static Data base[N], work[N], src[N];
    long long tot_comp[NALGO] = {0}, tot_move[NALGO] = {0};
    int all_ok = 1;

    printf("Data Count : %d\n\n", N);
    fflush(stdout);

    for (int t = 0; t < TRIALS; t++) {
        gen_random(base, N);
        if (t == 0) memcpy(src, base, sizeof(base));
        for (int k = 0; k < NALGO; k++) {
            long long c, m; int ok;
            run_one(ALGOS[k].fn, base, base, work, N, &c, &m, &ok);
            tot_comp[k] += c; tot_move[k] += m;
            if (!ok) all_ok = 0;
        }
        fprintf(stderr, "\rRandom trial %d / %d done", t + 1, TRIALS);
        fflush(stderr);
    }
    fprintf(stderr, "\n");

    printf("Random Data\n");
    printf("Number of Trials : %d\n\n", TRIALS);
    for (int k = 0; k < NALGO; k++) {
        printf("%s\n", ALGOS[k].name);
        printf("Total Comparisons   : %lld\n", tot_comp[k]);
        printf("Average Comparisons : %.2f\n", (double)tot_comp[k] / TRIALS);
        printf("Average Movements   : %.2f\n\n", (double)tot_move[k] / TRIALS);
    }
    printf("All %d trials sorted correctly : %s\n\n", TRIALS, all_ok ? "YES" : "NO");
    printf("------------------------------------\n\n");

    Data *sorted = (Data *)malloc(sizeof(Data) * N);
    Data *rev    = (Data *)malloc(sizeof(Data) * N);
    Data *part   = (Data *)malloc(sizeof(Data) * N);

    memcpy(sorted, src, sizeof(Data) * N);
    qsort(sorted, N, sizeof(Data), cmp_prep);

    for (int i = 0; i < N; i++) rev[i] = sorted[N - 1 - i];

    memcpy(part, sorted, sizeof(Data) * N);
    for (int s = 0; s < SWAP_COUNT; s++) {
        int i = rand_range(0, N - 1), j = rand_range(0, N - 1);
        Data tmp = part[i]; part[i] = part[j]; part[j] = tmp;
    }

    run_state("Sorted Data", sorted, src, work);
    run_state("Reverse-Sorted Data", rev, src, work);
    printf("[Partially Sorted Data 생성 방식] 정렬된 데이터에서 무작위 두 위치를 %d번(전체의 5%%) 교환\n\n", SWAP_COUNT);
    run_state("Partially Sorted Data", part, src, work);

    Data smalldata[4] = {{527, 100, 0}, {527, 200, 1}, {125, 300, 2}, {892, 400, 3}};

    enum { SN = 4, BN = 1000 };
    Data *dupdata = (Data *)malloc(sizeof(Data) * BN);
    for (int i = 0; i < BN; i++) {
        dupdata[i].key1  = rand_range(0, 49);
        dupdata[i].key2  = rand_range(0, MAXKEY);
        dupdata[i].order = i;
    }

    printf("Stability Verification\n\n");
    printf("Test data: (1) 과제 예시 4개  (2) key1 in [0,49]인 데이터 %d개 (order = 원래 입력 순서)\n", BN);
    print_arr("4-element original", smalldata, SN);
    printf("\n");
    stability_report("Selection Sort",      selection_sort,    smalldata, SN, dupdata, BN);
    stability_report("Bubble Sort",         bubble_sort,       smalldata, SN, dupdata, BN);
    stability_report("Bubble (Early Term)", bubble_sort_early, smalldata, SN, dupdata, BN);
    stability_report("Insertion Sort (>)",  insertion_sort_gt, smalldata, SN, dupdata, BN);
    stability_report("Insertion Sort (>=)", insertion_sort_ge, smalldata, SN, dupdata, BN);

    {
        Data *w = (Data *)malloc(sizeof(Data) * BN);
        memcpy(w, dupdata, sizeof(Data) * BN); g_comp = g_move = 0; insertion_sort_gt(w, BN);
        long long c1 = g_comp, m1 = g_move;
        memcpy(w, dupdata, sizeof(Data) * BN); g_comp = g_move = 0; insertion_sort_ge(w, BN);
        long long c2 = g_comp, m2 = g_move;
        printf("\nInsertion (>)  on dup data: comparisons=%lld, movements=%lld\n", c1, m1);
        printf("Insertion (>=) on dup data: comparisons=%lld, movements=%lld\n", c2, m2);
        free(w);
    }

    free(dupdata); free(sorted); free(rev); free(part);
    return 0;
}
