#include "filesystem.h"
#include "terminal.h"
#include "printf.h"

/* Simple static in-memory filesystem tree + trivial cwd tracking */

static const char README_CONTENT[] =
    "Welcome to hzOS in-memory FS.\n"
    "Try commands: help, lf, lf /etc, cat /etc/motd, pwd, cd /etc\n";

static const char MOTD_CONTENT[] =
    "hzOS Message of the Day:\n"
    "Have fun hacking your own kernel!\n";

static const char HOSTS_CONTENT[] =
    "127.0.0.1   localhost\n";

static const fs_node_t fs_root_children[];
static const fs_node_t fs_etc_children[];

static const fs_node_t fs_root = {
    .name = "/",
    .type = FS_NODE_DIR,
    .children = fs_root_children,
    .child_count = 3,
    .content = 0,
    .content_len = 0
};

static const fs_node_t* fs_cwd = &fs_root;
static char fs_cwd_path[64] = "/";

static const fs_node_t fs_root_children[] = {
    { "readme.txt", FS_NODE_FILE, 0, 0, README_CONTENT, sizeof(README_CONTENT) - 1 },
    { "bin",        FS_NODE_DIR,  0, 0, 0, 0 },
    { "etc",        FS_NODE_DIR,  fs_etc_children, 2, 0, 0 }
};

static const fs_node_t fs_etc_children[] = {
    { "motd",  FS_NODE_FILE, 0, 0, MOTD_CONTENT,  sizeof(MOTD_CONTENT) - 1 },
    { "hosts", FS_NODE_FILE, 0, 0, HOSTS_CONTENT, sizeof(HOSTS_CONTENT) - 1 }
};

void fs_init(void) {
    /* currently nothing dynamic beyond initial cwd */
    fs_cwd = &fs_root;
    fs_cwd_path[0] = '/';
    fs_cwd_path[1] = '\0';
    kprintf("fs: in-memory FS initialized\n");
}

const fs_node_t* fs_get_root(void) {
    return &fs_root;
}

static int path_is_root(const char* path) {
    return !path || path[0] == '\0' || (path[0] == '/' && path[1] == '\0');
}

static const fs_node_t* find_in_dir(const fs_node_t* dir, const char* name, size_t len) {
    if (!dir || dir->type != FS_NODE_DIR) return 0;
    for (size_t i = 0; i < dir->child_count; i++) {
        const fs_node_t* child = &dir->children[i];
        const char* cname = child->name;
        size_t j = 0;
        while (j < len && cname[j] && cname[j] == name[j]) {
            j++;
        }
        if (j == len && cname[j] == '\0') {
            return child;
        }
    }
    return 0;
}

const fs_node_t* fs_find(const char* path) {
    if (path_is_root(path)) {
        return &fs_root;
    }

    const fs_node_t* current = &fs_root;
    const char* p = path;

    if (*p == '/') p++;

    const char* segment_start = p;

    while (*p) {
        if (*p == '/') {
            size_t len = (size_t)(p - segment_start);
            if (len == 0) {
                segment_start = p + 1;
                p++;
                continue;
            }
            current = find_in_dir(current, segment_start, len);
            if (!current) return 0;
            segment_start = p + 1;
        }
        p++;
    }

    if (segment_start != p) {
        size_t len = (size_t)(p - segment_start);
        current = find_in_dir(current, segment_start, len);
    }

    return current;
}

void fs_list(const char* path) {
    const fs_node_t* node = fs_find(path);
    if (!node) {
        kprintf("lf: path not found: %s\n", path ? path : "/");
        return;
    }

    if (node->type != FS_NODE_DIR) {
        kprintf("lf: %s is not a directory\n", path && path[0] ? path : node->name);
        return;
    }

    for (size_t i = 0; i < node->child_count; i++) {
        const fs_node_t* child = &node->children[i];
        if (child->type == FS_NODE_DIR) {
            kprintf("[DIR]  %s\n", child->name);
        } else {
            kprintf("[FILE] %s (%u bytes)\n", child->name, (unsigned)child->content_len);
        }
    }
}

const char* fs_read(const char* path, size_t* out_len) {
    const fs_node_t* node = fs_find(path);
    if (!node) {
        return 0;
    }
    if (node->type != FS_NODE_FILE) {
        return 0;
    }
    if (out_len) {
        *out_len = node->content_len;
    }
    return node->content;
}

int fs_chdir(const char* path) {
    if (!path || !*path) {
        return 0;
    }

    /* Support absolute paths directly, and simple single-component relative
       paths from root (e.g. "etc" -> "/etc") for this tiny FS. */
    char tmp[64];
    const char* use = path;

    if (path[0] != '/') {
        tmp[0] = '/';
        size_t i = 0;
        while (path[i] && i + 1 < sizeof(tmp) - 1) {
            tmp[i + 1] = path[i];
            i++;
        }
        tmp[i + 1] = '\0';
        use = tmp;
    }

    const fs_node_t* node = fs_find(use);
    if (!node || node->type != FS_NODE_DIR) {
        return -1;
    }

    fs_cwd = node;

    /* Store the path we resolved (limited to our small tree). */
    size_t i = 0;
    while (use[i] && i < sizeof(fs_cwd_path) - 1) {
        fs_cwd_path[i] = use[i];
        i++;
    }
    fs_cwd_path[i] = '\0';

    return 0;
}

const char* fs_get_cwd(void) {
    return fs_cwd_path;
}

