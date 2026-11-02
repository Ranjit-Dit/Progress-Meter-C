#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define MAX_ITEMS 200
#define NAME_LEN 96
#define CATEGORY_LEN 48
#define DATE_LEN 11
#define DATA_FILE "progress_data.txt"
#define BAR_WIDTH 24

typedef struct {
    char category[CATEGORY_LEN];
    char name[NAME_LEN];
    char due_date[DATE_LEN];
    char notes[192];
    int complete;
    int paused;
    int priority; /* 1 = low, 2 = normal, 3 = high */
} Item;

static Item items[MAX_ITEMS];
static int item_count = 0;

static void trim_newline(char *text) { text[strcspn(text, "\r\n")] = '\0'; }

static void read_line(const char *prompt, char *out, size_t size) {
    printf("%s", prompt);
    if (!fgets(out, (int)size, stdin)) { out[0] = '\0'; return; }
    size_t length = strlen(out);
    int has_newline = length > 0 && out[length - 1] == '\n';
    trim_newline(out);
    if (!has_newline && length == size - 1) {
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF) {}
    }
}

static int valid_field(const char *value) { return value[0] && !strchr(value, '|'); }

static int valid_date(const char *date) {
    if (!date[0]) return 1;
    if (strlen(date) != 10 || date[4] != '-' || date[7] != '-') return 0;
    for (int i = 0; i < 10; i++)
        if (i != 4 && i != 7 && !isdigit((unsigned char)date[i])) return 0;
    int year = atoi(date), month = atoi(date + 5), day = atoi(date + 8);
    if (year < 1900 || month < 1 || month > 12) return 0;
    const int days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    int max_day = days[month - 1];
    if (month == 2 && (year % 400 == 0 || (year % 4 == 0 && year % 100 != 0))) max_day++;
    return day >= 1 && day <= max_day;
}

static int days_until(const char *date) {
    if (!date[0] || !valid_date(date)) return 999999;
    int year, month, day;
    sscanf(date, "%4d-%2d-%2d", &year, &month, &day);
    struct tm target = {0}, today = {0};
    target.tm_year = year - 1900; target.tm_mon = month - 1; target.tm_mday = day;
    time_t now = time(NULL);
    struct tm *local = localtime(&now);
    if (!local) return 999999;
    today = *local; today.tm_hour = 0; today.tm_min = 0; today.tm_sec = 0;
    target.tm_hour = 0;
    double seconds = difftime(mktime(&target), mktime(&today));
    return (int)(seconds / 86400.0 + (seconds >= 0 ? 0.5 : -0.5));
}

static void save_items(void) {
    FILE *file = fopen(DATA_FILE, "w");
    if (!file) { perror("Could not save progress"); return; }
    for (int i = 0; i < item_count; i++)
        fprintf(file, "%s|%s|%d|%s|%d|%d|%s\n", items[i].category, items[i].name,
                items[i].priority, items[i].due_date, items[i].complete, items[i].paused, items[i].notes);
    fclose(file);
}

/* Reads both the old category|name|complete format and the new five-field format. */
static void load_items(void) {
    FILE *file = fopen(DATA_FILE, "r");
    if (!file) return;
    char line[CATEGORY_LEN + NAME_LEN + 320];
    while (item_count < MAX_ITEMS && fgets(line, sizeof line, file)) {
        trim_newline(line);
        char *a = strchr(line, '|'); if (!a) continue; *a++ = '\0';
        char *b = strchr(a, '|'); if (!b) continue; *b++ = '\0';
        char *c = strchr(b, '|');
        Item item = {0};
        if (c) {
            *c++ = '\0';
            char *d = strchr(c, '|'); if (!d) continue; *d++ = '\0';
            item.priority = atoi(b);
            if (item.priority < 1 || item.priority > 3) item.priority = 2;
            snprintf(item.due_date, sizeof item.due_date, "%s", c);
            char *e = strchr(d, '|');
            if (e) {
                *e++ = '\0';
                char *f = strchr(e, '|');
                if (f) *f++ = '\0';
                item.complete = atoi(d) != 0;
                item.paused = atoi(e) != 0;
                if (f) snprintf(item.notes, sizeof item.notes, "%s", f);
            } else item.complete = atoi(d) != 0;
        } else {
            item.priority = 2;
            item.complete = atoi(b) != 0;
        }
        if (!valid_field(line) || !valid_field(a) || !valid_date(item.due_date)) continue;
        snprintf(item.category, sizeof item.category, "%s", line);
        snprintf(item.name, sizeof item.name, "%s", a);
        items[item_count++] = item;
    }
    fclose(file);
}

static void progress_bar(int done, int total) {
    int filled = total ? done * BAR_WIDTH / total : 0;
    putchar('[');
    for (int i = 0; i < BAR_WIDTH; i++) putchar(i < filled ? '#' : '-');
    printf("] %3d%%", total ? done * 100 / total : 0);
}

static const char *priority_label(int priority) {
    return priority == 3 ? "HIGH" : priority == 1 ? "LOW" : "NORMAL";
}

static void list_items(void) {
    if (!item_count) { puts("\nNo milestones yet. Choose Add milestone to get started."); return; }
    puts("\nYOUR MILESTONES");
    puts("--------------------------------------------------------------------------");
    for (int i = 0; i < item_count; i++) {
        const char *status = items[i].complete ? "DONE" : items[i].paused ? "PAUSED" : "OPEN";
        printf(" %3d. [%-6s] [%-6s] %-14s %s", i + 1, status,
               priority_label(items[i].priority), items[i].category, items[i].name);
        if (items[i].due_date[0]) {
            int left = days_until(items[i].due_date);
            if (!items[i].complete && left < 0) printf("  DUE %s (OVERDUE)", items[i].due_date);
            else if (!items[i].complete && left == 0) printf("  DUE TODAY (%s)", items[i].due_date);
            else if (!items[i].complete && left <= 7) printf("  DUE %s (%d day%s)", items[i].due_date, left, left == 1 ? "" : "s");
            else printf("  due %s", items[i].due_date);
        }
        if (items[i].notes[0]) printf("\n      Note: %s", items[i].notes);
        putchar('\n');
    }
    puts("--------------------------------------------------------------------------");
}

static int ask_item_number(const char *prompt) {
    if (!item_count) { puts("There are no milestones yet."); return -1; }
    list_items();
    char input[32]; read_line(prompt, input, sizeof input);
    char *end; long number = strtol(input, &end, 10);
    if (end == input || *end || number < 1 || number > item_count) {
        puts("Please enter a valid milestone number."); return -1;
    }
    return (int)number - 1;
}

static int ask_priority(int current) {
    char input[32];
    printf("Priority: 1=Low, 2=Normal, 3=High (current %s; Enter keeps it): ", priority_label(current));
    if (!fgets(input, sizeof input, stdin)) return current;
    trim_newline(input);
    if (!input[0]) return current;
    if (strcmp(input, "1") == 0 || strcmp(input, "2") == 0 || strcmp(input, "3") == 0) return atoi(input);
    puts("Invalid priority; keeping current value.");
    return current;
}

static void add_item(void) {
    if (item_count >= MAX_ITEMS) { puts("Milestone limit reached."); return; }
    Item item = {0};
    read_line("Category (e.g. Academic, Career, Personal): ", item.category, sizeof item.category);
    read_line("Milestone: ", item.name, sizeof item.name);
    if (!valid_field(item.category) || !valid_field(item.name)) {
        puts("Enter both values. The | character is not allowed."); return;
    }
    item.priority = ask_priority(2);
    read_line("Due date YYYY-MM-DD (optional; Enter skips): ", item.due_date, sizeof item.due_date);
    if (!valid_date(item.due_date)) { puts("Invalid date. Use a real date in YYYY-MM-DD format."); return; }
    read_line("Note (optional): ", item.notes, sizeof item.notes);
    if (strchr(item.notes, '|')) { puts("The | character is not allowed in notes."); return; }
    items[item_count++] = item;
    save_items(); puts("Milestone added.");
}

static void toggle_item(void) {
    int index = ask_item_number("Number to mark complete / reopen: ");
    if (index < 0) return;
    items[index].complete = !items[index].complete;
    if (items[index].complete) items[index].paused = 0;
    save_items();
    printf("Updated: %s — %s\n", items[index].name, items[index].complete ? "complete" : "in progress");
}

static void edit_item(void) {
    int index = ask_item_number("Number to edit: "); if (index < 0) return;
    char category[CATEGORY_LEN], name[NAME_LEN], date[DATE_LEN], notes[192];
    printf("Current category: %s\n", items[index].category);
    read_line("New category (Enter keeps current): ", category, sizeof category);
    printf("Current milestone: %s\n", items[index].name);
    read_line("New milestone (Enter keeps current): ", name, sizeof name);
    printf("Current due date: %s\n", items[index].due_date[0] ? items[index].due_date : "none");
    read_line("New due date YYYY-MM-DD (Enter keeps current, '-' clears): ", date, sizeof date);
    printf("Current note: %s\n", items[index].notes[0] ? items[index].notes : "none");
    read_line("New note (Enter keeps current, '-' clears): ", notes, sizeof notes);
    if ((category[0] && !valid_field(category)) || (name[0] && !valid_field(name)) ||
        (date[0] && strcmp(date, "-") != 0 && !valid_date(date)) || strchr(notes, '|')) {
        puts("Invalid input. No changes made."); return;
    }
    int priority = ask_priority(items[index].priority);
    if (category[0]) snprintf(items[index].category, CATEGORY_LEN, "%s", category);
    if (name[0]) snprintf(items[index].name, NAME_LEN, "%s", name);
    if (strcmp(date, "-") == 0) items[index].due_date[0] = '\0';
    else if (date[0]) snprintf(items[index].due_date, DATE_LEN, "%s", date);
    if (strcmp(notes, "-") == 0) items[index].notes[0] = '\0';
    else if (notes[0]) snprintf(items[index].notes, sizeof items[index].notes, "%s", notes);
    items[index].priority = priority;
    save_items(); puts("Milestone updated.");
}

static void delete_item(void) {
    int index = ask_item_number("Number to delete: "); if (index < 0) return;
    printf("Delete '%s'? Type y to confirm: ", items[index].name);
    char answer[16]; read_line("", answer, sizeof answer);
    if (tolower((unsigned char)answer[0]) != 'y') { puts("Deletion cancelled."); return; }
    for (int i = index; i < item_count - 1; i++) items[i] = items[i + 1];
    item_count--; save_items(); puts("Milestone deleted.");
}

static void pause_item(void) {
    int index = ask_item_number("Number to pause / resume: ");
    if (index < 0) return;
    if (items[index].complete) { puts("A completed milestone cannot be paused."); return; }
    items[index].paused = !items[index].paused;
    save_items();
    printf("%s is now %s.\n", items[index].name, items[index].paused ? "paused" : "active");
}

static void focus_view(void) {
    char choice[16];
    puts("\nFocus view: 1) Open  2) Paused  3) Completed  4) High priority  5) All");
    read_line("Choose a view: ", choice, sizeof choice);
    int shown = 0;
    for (int i = 0; i < item_count; i++) {
        int match = strcmp(choice, "5") == 0 ||
            (strcmp(choice, "1") == 0 && !items[i].complete && !items[i].paused) ||
            (strcmp(choice, "2") == 0 && items[i].paused) ||
            (strcmp(choice, "3") == 0 && items[i].complete) ||
            (strcmp(choice, "4") == 0 && items[i].priority == 3 && !items[i].complete);
        if (!match) continue;
        printf(" %3d. [%s] [%s] %s — %s", i + 1,
            items[i].complete ? "DONE" : items[i].paused ? "PAUSED" : "OPEN",
            priority_label(items[i].priority), items[i].category, items[i].name);
        if (items[i].due_date[0]) printf(" (due %s)", items[i].due_date);
        if (items[i].notes[0]) printf("\n      Note: %s", items[i].notes);
        putchar('\n'); shown++;
    }
    if (!shown) puts("No milestones match that view.");
}

static void search_items(void) {
    char query[NAME_LEN]; read_line("Search milestone or category: ", query, sizeof query);
    if (!query[0]) return;
    char needle[NAME_LEN]; snprintf(needle, sizeof needle, "%s", query);
    for (char *p = needle; *p; p++) *p = (char)tolower((unsigned char)*p);
    int found = 0; printf("\nMatches for '%s':\n", query);
    for (int i = 0; i < item_count; i++) {
        char name[NAME_LEN], category[CATEGORY_LEN];
        snprintf(name, sizeof name, "%s", items[i].name);
        snprintf(category, sizeof category, "%s", items[i].category);
        for (char *p = name; *p; p++) *p = (char)tolower((unsigned char)*p);
        for (char *p = category; *p; p++) *p = (char)tolower((unsigned char)*p);
        if (strstr(name, needle) || strstr(category, needle)) {
            printf(" %3d. [%c] [%-6s] %-14s %s\n", i + 1, items[i].complete ? 'x' : ' ',
                   priority_label(items[i].priority), items[i].category, items[i].name); found++;
        }
    }
    if (!found) puts("No matching milestones.");
}

static void show_summary(void) {
    if (!item_count) { puts("\nNo progress to summarize yet."); return; }
    int complete = 0, overdue = 0, due_soon = 0, paused = 0;
    for (int i = 0; i < item_count; i++) {
        complete += items[i].complete;
        paused += items[i].paused;
        if (!items[i].complete && items[i].due_date[0]) {
            int left = days_until(items[i].due_date);
            if (left < 0) overdue++;
            else if (left <= 7) due_soon++;
        }
    }
    puts("\nPROGRESS OVERVIEW");
    printf("Overall  "); progress_bar(complete, item_count);
    printf("  (%d/%d milestones)\n", complete, item_count);
    printf("Deadlines: %d overdue, %d due in the next 7 days\n", overdue, due_soon);
    printf("Status: %d active, %d paused, %d completed\n", item_count - complete - paused, paused, complete);
    puts("\nBY CATEGORY");
    for (int i = 0; i < item_count; i++) {
        int seen = 0;
        for (int j = 0; j < i; j++) if (strcmp(items[i].category, items[j].category) == 0) seen = 1;
        if (seen) continue;
        int total = 0, done = 0;
        for (int j = 0; j < item_count; j++) if (strcmp(items[i].category, items[j].category) == 0) {
            total++; done += items[j].complete;
        }
        printf("%-16s ", items[i].category); progress_bar(done, total); printf("  (%d/%d)\n", done, total);
    }
    if (complete == item_count) puts("\nEverything is complete. Great work!");
    else printf("\n%d milestone%s left to complete.\n", item_count - complete, item_count - complete == 1 ? "" : "s");
}

int main(void) {
    load_items();
    puts("============================================");
    puts("               PROGRESS METER               ");
    puts("============================================");
    for (;;) {
        puts("\n1) View milestones     2) Add milestone");
        puts("3) Toggle completion   4) Progress summary");
        puts("5) Edit milestone      6) Delete milestone");
        puts("7) Search              8) Quit");
        puts("9) Pause / resume      10) Focus view");
        char input[32]; read_line("Choose an option: ", input, sizeof input);
        if (strcmp(input, "1") == 0) list_items();
        else if (strcmp(input, "2") == 0) add_item();
        else if (strcmp(input, "3") == 0) toggle_item();
        else if (strcmp(input, "4") == 0) show_summary();
        else if (strcmp(input, "5") == 0) edit_item();
        else if (strcmp(input, "6") == 0) delete_item();
        else if (strcmp(input, "7") == 0) search_items();
        else if (strcmp(input, "8") == 0 || input[0] == '\0') break;
        else if (strcmp(input, "9") == 0) pause_item();
        else if (strcmp(input, "10") == 0) focus_view();
        else puts("Please choose 1 through 10.");
    }
    puts("Progress saved. See you next time.");
    return 0;
}
