#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "common.h"

typedef enum {
    FS_NODE_FILE,
    FS_NODE_DIR
} fs_node_type_t;

typedef struct fs_node {
    char name[32];
    fs_node_type_t type;
    struct fs_node* first_child;
    struct fs_node* next_sibling;
    struct fs_node* parent;
    char* content;
    size_t content_len;
} fs_node_t;

void fs_init(void);
fs_node_t* fs_get_root(void);
fs_node_t* fs_find(const char* path);
void fs_list(const char* path);
const char* fs_read(const char* path, size_t* out_len);
int fs_chdir(const char* path);
const char* fs_get_cwd(void);

/* New Dynamic Functions */
int fs_mkdir(const char* path);
int fs_mkfile(const char* path);
int fs_write_file(const char* path, const char* data, size_t len);

#endif