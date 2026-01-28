#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "common.h"

typedef enum {
    FS_NODE_FILE,
    FS_NODE_DIR
} fs_node_type_t;

typedef struct fs_node {
    const char* name;
    fs_node_type_t type;
    const struct fs_node* children;
    size_t child_count;
    const char* content;
    size_t content_len;
} fs_node_t;

void fs_init(void);
const fs_node_t* fs_get_root(void);
const fs_node_t* fs_find(const char* path);
void fs_list(const char* path);
const char* fs_read(const char* path, size_t* out_len);

#endif
