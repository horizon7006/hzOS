#include "filesystem.h"
#include "terminal.h"
#include "printf.h"
#include "../lib/memory.h"
#include "ext2.h"

static fs_node_t* fs_root = 0;
static fs_node_t* fs_cwd = 0;
static char fs_cwd_path[256] = "/";

/* Helper to create a new node */
static fs_node_t* create_node(const char* name, fs_node_type_t type, fs_node_t* parent) {
    fs_node_t* node = (fs_node_t*)kmalloc_z(sizeof(fs_node_t));
    
    // Copy name safely
    size_t i = 0;
    while (name[i] && i < 31) {
        node->name[i] = name[i];
        i++;
    }
    node->name[i] = '\0';
    
    node->type = type;
    node->parent = parent;
    
    return node;
}

/* Helper to add child to parent */
static void add_child(fs_node_t* parent, fs_node_t* child) {
    if (!parent || !child) return;
    
    if (!parent->first_child) {
        parent->first_child = child;
    } else {
        fs_node_t* n = parent->first_child;
        while (n->next_sibling) {
            n = n->next_sibling;
        }
        n->next_sibling = child;
    }
}

void fs_init(void) {
    fs_root = create_node("/", FS_NODE_DIR, 0);
    fs_cwd = fs_root;
    
    // Create default structure
    fs_node_t* readme = create_node("readme.txt", FS_NODE_FILE, fs_root);
    const char* readme_text = "Welcome to dynamic hzOS FS!\nTry mkdir, touch, write.\n";
    readme->content = (char*)kmalloc(128);
    // Copy content
    size_t ri = 0; 
    while (readme_text[ri]) { readme->content[ri] = readme_text[ri]; ri++; }
    readme->content[ri] = '\0';
    readme->content_len = ri;
    
    add_child(fs_root, readme);
    
    fs_node_t* bin = create_node("bin", FS_NODE_DIR, fs_root);
    add_child(fs_root, bin);
    
    fs_node_t* etc = create_node("etc", FS_NODE_DIR, fs_root);
    add_child(fs_root, etc);
    
    fs_node_t* motd = create_node("motd", FS_NODE_FILE, etc);
    const char* motd_text = "Have fun hacking!\n";
    motd->content = (char*)kmalloc(64);
    size_t mi = 0;
    while (motd_text[mi]) { motd->content[mi] = motd_text[mi]; mi++; }
    motd->content[mi] = 0;
    motd->content_len = mi;
    
    add_child(etc, motd);
    
    // Create /etc/hzsh.conf
    fs_node_t* hzsh_conf = create_node("hzsh.conf", FS_NODE_FILE, etc);
    const char* conf_content = 
        "# hzsh Configuration File\n"
        "# Shell Prompt\n"
        "prompt=> \n"
        "\n"
        "# Aliases\n"
        "alias ll=lf\n"
        "alias cls=clear\n";
    
    int conf_len = 0;
    while (conf_content[conf_len]) conf_len++;
    
    hzsh_conf->content = (char*)kmalloc(conf_len + 1);
    for (int i = 0; i <= conf_len; i++) {
        hzsh_conf->content[i] = conf_content[i];
    }
    hzsh_conf->content_len = conf_len;
    
    add_child(etc, hzsh_conf);

    kprintf("fs: dynamic FS initialized\n");
}

fs_node_t* fs_get_root(void) {
    return fs_root;
}

static fs_node_t* find_in_dir(fs_node_t* dir, const char* name, size_t len) {
    if (!dir || dir->type != FS_NODE_DIR) return 0;
    
    fs_node_t* child = dir->first_child;
    while (child) {
        // Compare names
        const char* cname = child->name;
        size_t j = 0;
        while (j < len && cname[j] && cname[j] == name[j]) {
            j++;
        }
        if (j == len && cname[j] == '\0') {
            return child;
        }
        child = child->next_sibling;
    }
    return 0;
}

fs_node_t* fs_find(const char* path) {
    if (!path || !*path) return fs_cwd; // Default to cwd? or 0? 
    
    // Handle root specially
    if (path[0] == '/' && path[1] == '\0') return fs_root;

    fs_node_t* current;
    const char* p = path;
    
    if (*p == '/') {
        current = fs_root;
        p++;
    } else {
        current = fs_cwd;
    }
    
    const char* seg_start = p;
    while (*p) {
        if (*p == '/') {
            size_t len = (size_t)(p - seg_start);
            if (len > 0) {
                // Check if ".."
                if (len == 2 && seg_start[0] == '.' && seg_start[1] == '.') {
                    if (current->parent) current = current->parent;
                } else if (len == 1 && seg_start[0] == '.') {
                    // Stay
                } else {
                    current = find_in_dir(current, seg_start, len);
                    if (!current) return 0;
                }
            }
            seg_start = p + 1;
        }
        p++;
    }
    
    if (p != seg_start) {
        size_t len = (size_t)(p - seg_start);
        if (len == 2 && seg_start[0] == '.' && seg_start[1] == '.') {
            if (current->parent) current = current->parent;
        } else if (len == 1 && seg_start[0] == '.') {
             // Stay
        } else {
            current = find_in_dir(current, seg_start, len);
        }
    }
    
    return current;
}

void fs_list(const char* path) {
    fs_node_t* node = fs_find(path);
    if (!node) {
        kprintf("lf: path not found: %s\n", path ? path : "(null)");
        return;
    }
    
    if (node->type != FS_NODE_DIR) {
        kprintf("lf: %s is not a directory\n", node->name);
        return;
    }
    
    fs_node_t* child = node->first_child;
    while (child) {
        if (child->type == FS_NODE_DIR) {
            kprintf("[DIR]  %s\n", child->name);
        } else {
             kprintf("[FILE] %s (%u bytes)\n", child->name, (unsigned)child->content_len);
        }
        child = child->next_sibling;
    }
}

const char* fs_read(const char* path, size_t* out_size) {
    if (!path) return NULL;
    
    // Try EXT2 first (check if mounted by seeing if we can find the inode)
    int inode_num = ext2_find_inode(path);
    if (inode_num > 0) {
        // Allocate buffer for file - assume max 64KB for now
        static uint8_t ext2_buffer[65536];
        int bytes_read = ext2_read_file(inode_num, ext2_buffer, sizeof(ext2_buffer));
        if (bytes_read > 0) {
            if (out_size) *out_size = bytes_read;
            return (const char*)ext2_buffer;
        }
    }
    
    // Fallback to in-memory VFS
    fs_node_t* node = fs_find(path);
    if (!node || node->type != FS_NODE_FILE) {
        return NULL;
    }
    
    if (out_size) {
        *out_size = node->content_len;
    }
    return node->content;
}

int fs_chdir(const char* path) {
    fs_node_t* node = fs_find(path);
    if (!node || node->type != FS_NODE_DIR) return -1;
    
    fs_cwd = node;
    
    // Reconstruct path string (hacky)
    // For now, if absolute, use it. If relative... complicated without ".."
    // Reset to "/" if root
    if (node == fs_root) {
        fs_cwd_path[0] = '/';
        fs_cwd_path[1] = '\0';
    } else {
        // Just store the last name for now or "..."
        // TODO: Full path reconstruction by walking up parents
        // This is a minimal implementation
        char tmp[256];
        int ptr = 255;
        tmp[ptr] = 0;
        
        fs_node_t* curr = fs_cwd;
        while(curr && curr != fs_root) {
            int len = 0;
             while(curr->name[len]) len++;
             
             ptr -= len;
             for(int i=0; i<len; i++) tmp[ptr+i] = curr->name[i];
             
             ptr--;
             tmp[ptr] = '/';
             
             curr = curr->parent;
        }
        
        // Copy to fs_cwd_path
        int i=0; 
        while(tmp[ptr]) {
            fs_cwd_path[i++] = tmp[ptr++];
        }
        fs_cwd_path[i] = 0;
        if (i==0) { fs_cwd_path[0]='/'; fs_cwd_path[1]=0; }
    }
    
    return 0;
}

const char* fs_get_cwd(void) {
    return fs_cwd_path;
}

/* --- New Dynamic Functions --- */

static void get_parent_and_name(const char* path, fs_node_t** parent_out, char* name_out) {
    // Find last slash
    const char* last_slash = 0;
    const char* p = path;
    while (*p) { 
        if (*p == '/') last_slash = p;
        p++;
    }
    
    fs_node_t* parent = 0;
    if (!last_slash) {
        // file in cwd
        parent = fs_cwd;
        // Copy name
        int i=0;
        while(path[i] && i<31) { name_out[i]=path[i]; i++; }
        name_out[i]=0;
    } else {
        // Has slash
        if (last_slash == path) {
             // "/file" -> parent is root
             parent = fs_root;
        } else {
             // "/a/b/file" -> parent is "/a/b"
             // Temporarily terminate info
             // This is tricky with const char.
             // We'll alloc a temp buffer for path
             int len = (int)(last_slash - path);
             char* tmp = (char*)kmalloc(len+1);
             for(int k=0; k<len; k++) tmp[k] = path[k];
             tmp[len] = 0;
             parent = fs_find(tmp);
             // leak tmp (simplification)
        }
        
        // Name is after slash
        const char* n = last_slash + 1;
        int i=0;
        while(n[i] && i<31) { name_out[i]=n[i]; i++; }
        name_out[i]=0;
    }
    
    *parent_out = parent;
}

int fs_mkdir(const char* path) {
    fs_node_t* parent;
    char name[32];
    get_parent_and_name(path, &parent, name);
    
    if (!parent || parent->type != FS_NODE_DIR) return -1;
    
    // Check exist (calculate actual length first)
    size_t name_len = 0;
    while (name[name_len]) name_len++;
    
    if (find_in_dir(parent, name, name_len)) return -1; // Already exists
    
    fs_node_t* node = create_node(name, FS_NODE_DIR, parent);
    add_child(parent, node);
    return 0;
}

int fs_mkfile(const char* path) {
    fs_node_t* parent;
    char name[32];
    get_parent_and_name(path, &parent, name);
    
    if (!parent || parent->type != FS_NODE_DIR) return -1;
    
    size_t name_len = 0;
    while (name[name_len]) name_len++;
    
    if (find_in_dir(parent, name, name_len)) return -1; 
    
    fs_node_t* node = create_node(name, FS_NODE_FILE, parent);
    add_child(parent, node);
    return 0;
}

int fs_write_file(const char* path, const char* data, size_t len) {
    fs_node_t* node = fs_find(path);
    if (!node) {
        // Try create
        if (fs_mkfile(path) != 0) return -1;
        node = fs_find(path);
        if (!node) return -1;
    }
    
    if (node->type != FS_NODE_FILE) return -1;
    
    // Allocate content (leak old content if exists - no free)
    char* buf = (char*)kmalloc(len + 1);
    for (size_t i=0; i<len; i++) buf[i] = data[i];
    buf[len] = 0;
    
    node->content = buf;
    node->content_len = len;
    
    return 0;
}

