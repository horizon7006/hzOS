#include "command.h"
#include "terminal.h"
#include "printf.h"
#include "filesystem.h"

typedef struct {
    const char* name;
    const char* help;
    command_func_t fn;
} command_entry_t;

static void cmd_help(const char* args);
static void cmd_clear(const char* args);
static void cmd_about(const char* args);
static void cmd_lf(const char* args);
static void cmd_cat(const char* args);

static command_entry_t commands[] = {
    { "help",  "Show available commands",      cmd_help  },
    { "clear", "Clear the screen",             cmd_clear },
    { "about", "Show information about hzOS",  cmd_about },
    { "list",    "List directory contents",      cmd_lf    },
    { "cat",   "Print file contents",          cmd_cat   },
};

static const size_t command_count = sizeof(commands) / sizeof(commands[0]);

void command_init(void) {
    kprintf("commands: %u registered\n", (unsigned)command_count);
}

static void skip_spaces(const char** s) {
    while (**s == ' ' || **s == '\t') {
        (*s)++;
    }
}

static int str_eq(const char* a, const char* b) {
    while (*a && *b && *a == *b) {
        a++;
        b++;
    }
    return *a == '\0' && *b == '\0';
}

void command_execute(const char* line) {
    if (!line) return;

    const char* p = line;
    skip_spaces(&p);

    if (*p == '\0') return;

    const char* name_start = p;
    while (*p && *p != ' ' && *p != '\t') {
        p++;
    }

    size_t name_len = (size_t)(p - name_start);

    skip_spaces(&p);
    const char* args = p;

    for (size_t i = 0; i < command_count; i++) {
        const char* cname = commands[i].name;
        size_t j = 0;
        while (j < name_len && cname[j] && cname[j] == name_start[j]) {
            j++;
        }
        if (j == name_len && cname[j] == '\0') {
            commands[i].fn(args);
            return;
        }
    }

    kprintf("Unknown command: %s\n", line);
}

static void cmd_help(const char* args) {
    (void)args;
    terminal_writeln("Available commands:");
    for (size_t i = 0; i < command_count; i++) {
        kprintf("  %s - %s\n", commands[i].name, commands[i].help);
    }
}

static void cmd_clear(const char* args) {
    (void)args;
    terminal_clear();
}

static void cmd_about(const char* args) {
    (void)args;
    terminal_writeln("hzOS: tiny hobby kernel with a toy shell and in-memory FS.");
    terminal_writeln("Build info: simple 32-bit x86, BIOS, Multiboot.");
}

static void cmd_lf(const char* args) {
    const char* path = args;
    if (!path) {
        fs_list("/");
        return;
    }

    while (*path == ' ' || *path == '\t') {
        path++;
    }

    if (*path == '\0') {
        fs_list("/");
    } else {
        fs_list(path);
    }
}

static void cmd_cat(const char* args) {
    const char* path = args;
    if (!path) {
        kprintf("cat: path required\n");
        return;
    }

    while (*path == ' ' || *path == '\t') {
        path++;
    }

    if (*path == '\0') {
        kprintf("cat: path required\n");
        return;
    }

    size_t len = 0;
    const char* data = fs_read(path, &len);
    if (!data) {
        kprintf("cat: cannot open %s\n", path);
        return;
    }

    for (size_t i = 0; i < len; i++) {
        terminal_putc(data[i]);
    }
    terminal_putc('\n');
}

