#ifndef HZSH_CONFIG_H
#define HZSH_CONFIG_H

#define MAX_PROMPT_LEN 64
#define MAX_ALIAS_NAME 32
#define MAX_ALIAS_VALUE 128
#define MAX_ALIASES 32

typedef struct {
    char name[MAX_ALIAS_NAME];
    char value[MAX_ALIAS_VALUE];
} alias_t;

typedef struct {
    char prompt[MAX_PROMPT_LEN];
    int color_prompt;
    int history_size;
    alias_t aliases[MAX_ALIASES];
    int alias_count;
} hzsh_config_t;

/* Load configuration from /etc/hzsh.conf */
void hzsh_config_load(hzsh_config_t* config);

/* Get default configuration */
void hzsh_config_default(hzsh_config_t* config);

/* Resolve alias if it exists */
const char* hzsh_alias_resolve(hzsh_config_t* config, const char* name);

#endif
