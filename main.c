#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ncurses.h>
#include <dirent.h>
#include <ctype.h>

// ─── Structs ───────────────────────────────────────────────
typedef struct {
    long user, nice, system, idle, iowait, irq, softirq;
} CpuStat;

typedef struct {
    int   pid;
    char  name[256];
    float mem;
} Process;

// ─── CPU ───────────────────────────────────────────────────
CpuStat read_cpu_stat() {
    CpuStat s = {0};
    FILE *f = fopen("/proc/stat", "r");
    if (!f) return s;
    fscanf(f, "cpu %ld %ld %ld %ld %ld %ld %ld",
           &s.user, &s.nice, &s.system, &s.idle,
           &s.iowait, &s.irq, &s.softirq);
    fclose(f);
    return s;
}

float get_cpu_usage() {
    CpuStat a = read_cpu_stat();
    usleep(500000);
    CpuStat b = read_cpu_stat();

    long idle_a  = a.idle + a.iowait;
    long idle_b  = b.idle + b.iowait;
    long total_a = a.user + a.nice + a.system + a.idle + a.iowait + a.irq + a.softirq;
    long total_b = b.user + b.nice + b.system + b.idle + b.iowait + b.irq + b.softirq;

    long total_diff = total_b - total_a;
    long idle_diff  = idle_b  - idle_a;

    if (total_diff == 0) return 0.0f;
    return (float)(total_diff - idle_diff) / (float)total_diff * 100.0f;
}

// ─── RAM ───────────────────────────────────────────────────
void get_ram(long *used_mb, long *total_mb) {
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) return;

    long total = 0, free = 0, buffers = 0, cached = 0, sreclaimable = 0;
    char line[256];

    while (fgets(line, sizeof(line), f)) {
        long val;
        if (sscanf(line, "MemTotal: %ld", &val) == 1)     total        = val;
        if (sscanf(line, "MemFree: %ld", &val) == 1)      free         = val;
        if (sscanf(line, "Buffers: %ld", &val) == 1)      buffers      = val;
        if (sscanf(line, "Cached: %ld", &val) == 1)       cached       = val;
        if (sscanf(line, "SReclaimable: %ld", &val) == 1) sreclaimable = val;
    }
    fclose(f);

    *total_mb = total / 1024;
    *used_mb  = (total - free - buffers - cached - sreclaimable) / 1024;
    if (*used_mb < 0) *used_mb = 0;
}

// ─── Uptime ────────────────────────────────────────────────
void get_uptime(int *hours, int *minutes, int *seconds) {
    FILE *f = fopen("/proc/uptime", "r");
    if (!f) return;
    double up;
    fscanf(f, "%lf", &up);
    fclose(f);
    int total = (int)up;
    *hours   = total / 3600;
    *minutes = (total % 3600) / 60;
    *seconds = total % 60;
}

// ─── Processes ─────────────────────────────────────────────
int get_processes(Process *procs, int max) {
    DIR *dir = opendir("/proc");
    if (!dir) return 0;

    struct dirent *entry;
    int count = 0;
    long total_ram_kb = 0;

    FILE *mf = fopen("/proc/meminfo", "r");
    if (mf) { fscanf(mf, "%*s %ld", &total_ram_kb); fclose(mf); }

    while ((entry = readdir(dir)) != NULL && count < max) {
        if (!isdigit(entry->d_name[0])) continue;

        int pid = atoi(entry->d_name);
        char path[256];

        snprintf(path, sizeof(path), "/proc/%d/comm", pid);
        FILE *f = fopen(path, "r");
        if (!f) continue;
        char name[256];
        if (fscanf(f, "%255s", name) != 1) { fclose(f); continue; }
        fclose(f);

        snprintf(path, sizeof(path), "/proc/%d/status", pid);
        f = fopen(path, "r");
        long vmrss = 0;
        if (f) {
            char line[256];
            while (fgets(line, sizeof(line), f)) {
                if (strncmp(line, "VmRSS:", 6) == 0) {
                    sscanf(line, "VmRSS: %ld", &vmrss);
                    break;
                }
            }
            fclose(f);
        }

        procs[count].pid = pid;
        strncpy(procs[count].name, name, 255);
        procs[count].name[255] = '\0';
        procs[count].mem = total_ram_kb > 0
                           ? (float)vmrss / (float)total_ram_kb * 100.0f
                           : 0.0f;
        count++;
    }
    closedir(dir);
    return count;
}

// ─── Draw bar ──────────────────────────────────────────────
// '#' = filled, '-' = empty (no spaces so terminal can't collapse them)
void draw_bar(int y, int x, float pct, int width, int color_filled, int color_empty) {
    if (width <= 0 || width > 120) return;

    int filled = (int)(pct / 100.0f * (float)width);
    if (filled > width) filled = width;
    if (filled < 0)     filled = 0;

    mvprintw(y, x, "[");

    attron(COLOR_PAIR(color_filled));
    for (int i = 0; i < filled; i++)
        mvprintw(y, x + 1 + i, "#");
    attroff(COLOR_PAIR(color_filled));

    attron(COLOR_PAIR(color_empty));
    for (int i = filled; i < width; i++)
        mvprintw(y, x + 1 + i, "-");
    attroff(COLOR_PAIR(color_empty));

    mvprintw(y, x + 1 + width, "]");
}

// ─── Main ──────────────────────────────────────────────────
int main() {
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    start_color();
    nodelay(stdscr, TRUE);

    init_pair(1, COLOR_GREEN,  COLOR_BLACK); // green  — low usage filled
    init_pair(2, COLOR_YELLOW, COLOR_BLACK); // yellow — mid usage filled
    init_pair(3, COLOR_RED,    COLOR_BLACK); // red    — high usage filled
    init_pair(4, COLOR_CYAN,   COLOR_BLACK); // cyan   — headers
    init_pair(5, COLOR_WHITE,  COLOR_BLACK); // white  — normal text
    init_pair(6, COLOR_BLACK,  COLOR_BLACK); // dark   — empty bar (shows as dim dashes)

    // make pair 6 actually visible as dark gray
    if (can_change_color()) {
        init_color(COLOR_BLACK, 300, 300, 300);
    }

    Process procs[256];

    const int BAR_X     = 7;
    const int BAR_WIDTH = 50;
    const int LABEL_X   = BAR_X + BAR_WIDTH + 3;

    while (1) {
        clear();
        int rows, cols;
        getmaxyx(stdscr, rows, cols);

        // ── Header
        const char *title = " SYSTEM MONITOR ";
        int title_x = (cols - (int)strlen(title)) / 2;
        attron(COLOR_PAIR(4) | A_BOLD);
        mvprintw(0, title_x > 0 ? title_x : 0, "%s", title);
        attroff(COLOR_PAIR(4) | A_BOLD);
        mvhline(1, 0, ACS_HLINE, cols);

        // ── CPU
        float cpu = get_cpu_usage();
        int cpu_color = cpu < 50 ? 1 : cpu < 80 ? 2 : 3;
        attron(A_BOLD); mvprintw(3, 2, "CPU:"); attroff(A_BOLD);
        draw_bar(3, BAR_X, cpu, BAR_WIDTH, cpu_color, 5);
        attron(COLOR_PAIR(cpu_color) | A_BOLD);
        mvprintw(3, LABEL_X, "%.1f%%", cpu);
        attroff(COLOR_PAIR(cpu_color) | A_BOLD);

        // ── RAM
        long used_mb = 0, total_mb = 0;
        get_ram(&used_mb, &total_mb);
        float ram_pct = total_mb > 0 ? (float)used_mb / (float)total_mb * 100.0f : 0;
        int ram_color = ram_pct < 50 ? 1 : ram_pct < 80 ? 2 : 3;
        attron(A_BOLD); mvprintw(5, 2, "RAM:"); attroff(A_BOLD);
        draw_bar(5, BAR_X, ram_pct, BAR_WIDTH, ram_color, 5);
        attron(COLOR_PAIR(ram_color) | A_BOLD);
        mvprintw(5, LABEL_X, "%ld / %ld MB (%.1f%%)", used_mb, total_mb, ram_pct);
        attroff(COLOR_PAIR(ram_color) | A_BOLD);

        // ── Uptime
        int h, m, s;
        get_uptime(&h, &m, &s);
        attron(A_BOLD); mvprintw(7, 2, "Uptime:"); attroff(A_BOLD);
        attron(COLOR_PAIR(4));
        mvprintw(7, 10, "%02d:%02d:%02d", h, m, s);
        attroff(COLOR_PAIR(4));

        // ── Processes header
        mvhline(9,  0, ACS_HLINE, cols);
        attron(COLOR_PAIR(4) | A_BOLD);
        mvprintw(10, 2, "%-6s %-20s %8s", "PID", "NAME", "MEM%");
        attroff(COLOR_PAIR(4) | A_BOLD);
        mvhline(11, 0, ACS_HLINE, cols);

        // ── Process list
        int proc_count = get_processes(procs, 256);
        int max_show   = rows - 14;
        if (max_show < 0) max_show = 0;

        for (int i = 0; i < proc_count && i < max_show; i++) {
            int color = procs[i].mem > 5.0f ? 3 : procs[i].mem > 2.0f ? 2 : 5;
            attron(COLOR_PAIR(color));
            mvprintw(12 + i, 2, "%-6d %-20s %7.1f%%",
                     procs[i].pid, procs[i].name, procs[i].mem);
            attroff(COLOR_PAIR(color));
        }

        // ── Footer
        mvhline(rows - 2, 0, ACS_HLINE, cols);
        attron(COLOR_PAIR(4));
        mvprintw(rows - 1, 2, "Press Q to quit  |  Processes: %d  |  Uptime: %02d:%02d:%02d",
                 proc_count, h, m, s);
        attroff(COLOR_PAIR(4));

        refresh();

        int ch = getch();
        if (ch == 'q' || ch == 'Q') break;

        sleep(1);
    }

    endwin();
    return 0;
}
