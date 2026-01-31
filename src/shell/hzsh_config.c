#include "hzsh_config.h"
#include "../fs/filesystem.h"
#include "../lib/printf.h"
#include <stddef.h>

void hzsh_config_default(hzsh_config_t* config) {
    // Set default prompt
    const char* default_prompt = "> ";
    int i = 0;
    while (default_prompt[i] && i < MAX_PROMPT_LEN - 1) {
        config->prompt[i] = default_prompt[i];
        i++;
    }
    config->prompt[i] = 0;
    
    config->color_prompt = 0;
    config->history_size = 100;
    config->alias_count = 0;
}

static void parse_line(hzsh_config_t* config, const char* line) {
    // Skip empty lines and comments
    if (!line || *line == '\0' || *line == '#') return;
    
    // Skip leading whitespace
    while (*line == ' ' || *line == '\t') line++;
    if (*line == '\0' || *line == '#') return;
    
    // Check for "prompt="
    const char* key = "prompt=";
    int match = 1;
    for (int i = 0; key[i]; i++) {
        if (line[i] != key[i]) {
            match = 0;
            break;
        }
    }
    if (match) {
        const char* value = line + 7; // Skip "prompt="
        int i = 0;
        while (value[i] && i < MAX_PROMPT_LEN - 1) {
            config->prompt[i] = value[i];
            i++;
        }
        config->prompt[i] = 0;
        return;
    }
    
    // Check for "alias name=value"
    key = "alias ";
    match = 1;
    for (int i = 0; key[i]; i++) {
        if (line[i] != key[i]) {
            match = 0;
            break;
        }
    }
    if (match && config->alias_count < MAX_ALIASES) {
        const char* alias_def = line + 6; // Skip "alias "
        
        // Parse name=value
        int i = 0;
        while (alias_def[i] && alias_def[i] != '=' && i < MAX_ALIAS_NAME - 1) {
            config->aliases[config->alias_count].name[i] = alias_def[i];
            i++;
        }
        config->aliases[config->alias_count].name[i] = 0;
        
        if (alias_def[i] == '=') {
            i++; // Skip '='
            int j = 0;
            while (alias_def[i] && j < MAX_ALIAS_VALUE - 1) {
                config->aliases[config->alias_count].value[j] = alias_def[i];
                i++;
                j++;
            }
            config->aliases[config->alias_count].value[j] = 0;
            config->alias_count++;
        }
        return;
    }
}

void hzsh_config_load(hzsh_config_t* config) {
    // Set defaults first
    hzsh_config_default(config);
    
    // Try to read config file
    size_t size = 0;
    const char* content = fs_read("/etc/hzsh.conf", &size);
    if (!content) {
        return; // Use defaults if file doesn't exist
    }
    
    // Parse line by line
    char line[256];
    int line_pos = 0;
    for (size_t i = 0; i < size; i++) {
        char c = content[i];
        if (c == '\n' || c == '\r') {
            if (line_pos > 0) {
                line[line_pos] = 0;
                parse_line(config, line);
                line_pos = 0;
            }
        } else if (line_pos < 255) {
            line[line_pos++] = c;
        }
    }
    
    // Parse last line if no trailing newline
    if (line_pos > 0) {
        line[line_pos] = 0;
        parse_line(config, line);
    }
}

const char* hzsh_alias_resolve(hzsh_config_t* config, const char* name) {
    for (int i = 0; i < config->alias_count; i++) {
        const char* alias_name = config->aliases[i].name;
        int match = 1;
        int j = 0;
        while (alias_name[j] && name[j]) {
            if (alias_name[j] != name[j]) {
                match = 0;
                break;
            }
            j++;
        }
        if (match && alias_name[j] == '\0' && (name[j] == '\0' || name[j] == ' ')) {
            return config->aliases[i].value;
        }
    }
    return NULL; // No alias found
}
