#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdbool.h>

#define UVM_VERSION "0.1.0"
#define PACKAGE_FILE "uvmpackage.json"
#define MODS_DIR "umods"
#define GH_ORG_URL "https://github.com/unnarize"

// --- Forward Declarations ---
void handle_init();
void handle_get(const char* repo_name);
void handle_install();
void handle_uninstall(const char* repo_name);
void handle_version();
void print_usage();
int run_command(const char* command);
char* read_file_content(const char* path);
int write_file_content(const char* path, const char* content);
void fetch_and_clean_repo(const char* repo_name);

// --- Main Function (Command Dispatcher) ---
int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    const char* command = argv[1];

    if (strcmp(command, "init") == 0) {
        handle_init();
    } else if (strcmp(command, "get") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Error: 'get' command requires a repository name.\n");
            print_usage();
            return 1;
        }
        handle_get(argv[2]);
    } else if (strcmp(command, "install") == 0) {
        handle_install();
    } else if (strcmp(command, "uninstall") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Error: 'uninstall' command requires a repository name.\n");
            print_usage();
            return 1;
        }
        handle_uninstall(argv[2]);
    } else if (strcmp(command, "-v") == 0 || strcmp(command, "--version") == 0) {
        handle_version();
    } else {
        fprintf(stderr, "Error: Unknown command '%s'.\n", command);
        print_usage();
        return 1;
    }

    return 0;
}

// --- Command Handlers ---

/**
 * @brief Creates default project files: uvmpackage.json and .gitattributes.
 */
void handle_init() {
    // 1. Create uvmpackage.json
    if (access(PACKAGE_FILE, F_OK) == 0) {
        printf("'%s' already exists, skipping.\n", PACKAGE_FILE);
    } else {
        const char* package_content = "{\n"
                                      "  \"name\": \"my-unnarize-project\",\n"
                                      "  \"dependencies\": []\n"
                                      "}\n";

        if (write_file_content(PACKAGE_FILE, package_content) == 0) {
            printf("Initialized project with '%s'.\n", PACKAGE_FILE);
        } else {
            fprintf(stderr, "Error: Failed to create '%s'.\n", PACKAGE_FILE);
        }
    }

    // 2. Create .gitattributes for language detection
    const char* gitattributes_file = ".gitattributes";
    if (access(gitattributes_file, F_OK) == 0) {
        printf("'%s' already exists, skipping.\n", gitattributes_file);
    } else {
        const char* gitattributes_content = "# Tell GitHub's Linguist how to classify .gi files\n"
                                            "*.gi linguist-language=Unnarize\n";

        if (write_file_content(gitattributes_file, gitattributes_content) == 0) {
            printf("Created '%s' for GitHub language detection.\n", gitattributes_file);
        } else {
            fprintf(stderr, "Error: Failed to create '%s'.\n", gitattributes_file);
        }
    }
}


/**
 * @brief Fetches a repository and adds it to the dependencies in uvmpackage.json.
 */
void handle_get(const char* repo_name) {
    if (access(PACKAGE_FILE, F_OK) != 0) {
        fprintf(stderr, "Error: No '%s' found. Please run 'uvm init' first.\n", PACKAGE_FILE);
        return;
    }

    fetch_and_clean_repo(repo_name);

    char* json_content = read_file_content(PACKAGE_FILE);
    if (!json_content) {
        fprintf(stderr, "Error: Could not read '%s'.\n", PACKAGE_FILE);
        return;
    }

    if (strstr(json_content, repo_name)) {
        printf("Repository '%s' is already a dependency.\n", repo_name);
        free(json_content);
        return;
    }

    char* deps_start = strstr(json_content, "\"dependencies\": [");
    if (!deps_start) { fprintf(stderr, "Error: Invalid '%s' format.\n", PACKAGE_FILE); free(json_content); return; }

    char* insert_pos = strrchr(deps_start, ']');
    if (!insert_pos) { fprintf(stderr, "Error: Invalid '%s' format.\n", PACKAGE_FILE); free(json_content); return; }
    
    char* check_empty = strchr(deps_start, '[');
    bool is_empty = (insert_pos == check_empty + 1 || insert_pos == check_empty + 2);

    size_t new_size = strlen(json_content) + strlen(repo_name) + 16;
    char* new_json_content = malloc(new_size);
    if (!new_json_content) { fprintf(stderr, "Error: Memory allocation failed.\n"); free(json_content); return; }

    strncpy(new_json_content, json_content, insert_pos - json_content);
    new_json_content[insert_pos - json_content] = '\0';

    if (is_empty) {
        snprintf(new_json_content + strlen(new_json_content), new_size - strlen(new_json_content),
                 "\n    \"%s\"\n  ", repo_name);
    } else {
        snprintf(new_json_content + strlen(new_json_content), new_size - strlen(new_json_content),
                 ",\n    \"%s\"\n  ", repo_name);
    }
    
    strcat(new_json_content, insert_pos);

    if (write_file_content(PACKAGE_FILE, new_json_content) == 0) {
        printf("Added '%s' to dependencies.\n", repo_name);
    } else {
        fprintf(stderr, "Error: Failed to update '%s'.\n", PACKAGE_FILE);
    }

    free(json_content);
    free(new_json_content);
}

/**
 * @brief Installs all dependencies listed in uvmpackage.json.
 */
void handle_install() {
    if (access(PACKAGE_FILE, F_OK) != 0) {
        fprintf(stderr, "Error: No '%s' found. Please run 'uvm init' first.\n", PACKAGE_FILE);
        return;
    }

    char* json_content = read_file_content(PACKAGE_FILE);
    if (!json_content) return;

    char* deps_start = strstr(json_content, "\"dependencies\": [");
    if (!deps_start) { fprintf(stderr, "Error: Invalid '%s' format.\n", PACKAGE_FILE); free(json_content); return; }
    
    char* current = deps_start + strlen("\"dependencies\": [");
    char repo_buffer[128];
    int installed_count = 0;

    printf("Installing dependencies...\n");
    while (*current && *current != ']') {
        if (*current == '"') {
            char* end_quote = strchr(current + 1, '"');
            if (end_quote) {
                size_t len = end_quote - (current + 1);
                if (len < sizeof(repo_buffer) - 1) {
                    strncpy(repo_buffer, current + 1, len);
                    repo_buffer[len] = '\0';
                    fetch_and_clean_repo(repo_buffer);
                    installed_count++;
                }
                current = end_quote + 1;
            } else { break; }
        } else { current++; }
    }
    
    printf("\nInstallation complete. Installed %d package(s).\n", installed_count);
    free(json_content);
}

/**
 * @brief Uninstalls a dependency: removes its folder and updates uvmpackage.json.
 */
void handle_uninstall(const char* repo_name) {
    if (access(PACKAGE_FILE, F_OK) != 0) {
        fprintf(stderr, "Error: No '%s' found.\n", PACKAGE_FILE);
        return;
    }

    // 1. Remove the directory from umods/
    char dest_path[256];
    snprintf(dest_path, sizeof(dest_path), "%s/%s", MODS_DIR, repo_name);
    struct stat st;
    if (stat(dest_path, &st) == 0) {
        printf("Removing directory '%s'...\n", dest_path);
        char* command = NULL;
        int len = snprintf(NULL, 0, "rm -rf %s", dest_path);
        command = malloc(len + 1);
        snprintf(command, len + 1, "rm -rf %s", dest_path);
        run_command(command);
        free(command);
    } else {
        printf("Directory for '%s' not found locally. Checking dependencies file...\n", repo_name);
    }

    // 2. Remove the dependency from uvmpackage.json
    char* json_content = read_file_content(PACKAGE_FILE);
    if (!json_content) { fprintf(stderr, "Error: Could not read '%s'.\n", PACKAGE_FILE); return; }
    
    char search_str[256];
    snprintf(search_str, sizeof(search_str), "\"%s\"", repo_name);
    if (!strstr(json_content, search_str)) {
        printf("'%s' is not listed as a dependency. Nothing to do.\n", repo_name);
        free(json_content);
        return;
    }

    char* new_json_content = malloc(strlen(json_content) + 1);
    char* writer = new_json_content;
    char* deps_start = strstr(json_content, "\"dependencies\": [");
    
    // Copy everything before the dependencies array
    strncpy(writer, json_content, deps_start - json_content);
    writer += deps_start - json_content;
    
    // Copy the "dependencies": [ part
    const char* deps_header = "\"dependencies\": [";
    strcpy(writer, deps_header);
    writer += strlen(deps_header);
    
    // Iterate and rebuild the dependencies list
    char* current = deps_start + strlen(deps_header);
    char repo_buffer[128];
    bool is_first_entry = true;
    while (*current && *current != ']') {
        if (*current == '"') {
            char* end_quote = strchr(current + 1, '"');
            if (end_quote) {
                size_t len = end_quote - (current + 1);
                if (len < sizeof(repo_buffer) - 1) {
                    strncpy(repo_buffer, current + 1, len);
                    repo_buffer[len] = '\0';
                    if (strcmp(repo_name, repo_buffer) != 0) {
                        if (!is_first_entry) {
                            *writer++ = ',';
                        }
                        writer += sprintf(writer, "\n    \"%s\"", repo_buffer);
                        is_first_entry = false;
                    }
                }
                current = end_quote + 1;
            } else { break; }
        } else { current++; }
    }

    // Add closing bracket and brace
    if (!is_first_entry) {
        writer += sprintf(writer, "\n  ");
    }
    *writer++ = ']';
    *writer++ = '\n';
    *writer++ = '}';
    *writer++ = '\n';
    *writer = '\0';

    if (write_file_content(PACKAGE_FILE, new_json_content) == 0) {
        printf("Successfully removed '%s' from dependencies.\n", repo_name);
    } else {
        fprintf(stderr, "Error: Failed to update '%s'.\n", PACKAGE_FILE);
    }

    free(json_content);
    free(new_json_content);
}


/**
 * @brief Prints the uvm version.
 */
void handle_version() {
    printf("uvm version %s\n", UVM_VERSION);
}

/**
 * @brief Prints usage instructions.
 */
void print_usage() {
    printf("Unnarize Verse Manager (uvm) v%s\n", UVM_VERSION);
    printf("Usage: uvm <command> [options]\n\n");
    printf("Commands:\n");
    printf("  init                   Initialize a new Unnarize project.\n");
    printf("  get <repo-name>        Fetch a repository and add it to dependencies.\n");
    printf("  install                Install all dependencies from uvmpackage.json.\n");
    printf("  uninstall <repo-name>  Remove a repository from the project.\n");
    printf("  -v, --version          Show the uvm version.\n");
}


// --- Helper Functions ---

/**
 * @brief Runs a shell command and prints it to stdout first.
 * @return The exit code of the executed command.
 */
int run_command(const char* command) {
    printf("=> %s\n", command);
    return system(command);
}

/**
 * @brief Fetches a repository from GitHub, places it in the umods/ directory, and cleans it up.
 */
void fetch_and_clean_repo(const char* repo_name) {
    char dest_path[256];
    snprintf(dest_path, sizeof(dest_path), "%s/%s", MODS_DIR, repo_name);

    struct stat st_repo = {0};
    if (stat(dest_path, &st_repo) == 0) {
        printf("Repository '%s' already exists locally. Skipping download.\n", repo_name);
        return;
    }

    printf("\n--- Fetching '%s' ---\n", repo_name);

    struct stat st_umods = {0};
    if (stat(MODS_DIR, &st_umods) == -1) {
        mkdir(MODS_DIR, 0755);
    }
    
    char repo_url[256];
    snprintf(repo_url, sizeof(repo_url), "%s/%s.git", GH_ORG_URL, repo_name);

    int git_cmd_len = snprintf(NULL, 0, "git clone --depth 1 %s %s", repo_url, dest_path);
    char *git_command = malloc(git_cmd_len + 1);
    if (!git_command) { fprintf(stderr, "Error: Memory allocation failed.\n"); return; }
    snprintf(git_command, git_cmd_len + 1, "git clone --depth 1 %s %s", repo_url, dest_path);

    int result = run_command(git_command);
    free(git_command); 

    if (result != 0) {
        fprintf(stderr, "Error: Failed to get repository '%s'. Please check the name.\n", repo_name);
        return;
    }

    int cleanup_cmd_len = snprintf(NULL, 0, "rm -rf %s/.git %s/.vscode", dest_path, dest_path);
    char *cleanup_command = malloc(cleanup_cmd_len + 1);
    if (!cleanup_command) { fprintf(stderr, "Error: Memory allocation failed.\n"); return; }
    snprintf(cleanup_command, cleanup_cmd_len + 1, "rm -rf %s/.git %s/.vscode", dest_path, dest_path);
    
    printf("Cleaning up repository files...\n");
    run_command(cleanup_command);
    free(cleanup_command);

    printf("Successfully installed '%s' into %s/\n", repo_name, MODS_DIR);
}


/**
 * @brief Reads the entire content of a file into a dynamically allocated string.
 * @param path The path to the file.
 * @return A pointer to the content string, or NULL on failure. The caller must free this memory.
 */
char* read_file_content(const char* path) {
    FILE* file = fopen(path, "rb");
    if (!file) return NULL;

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = malloc(length + 1);
    if (buffer) {
        fread(buffer, 1, length, file);
        buffer[length] = '\0';
    }
    fclose(file);
    return buffer;
}

/**
 * @brief Writes a string to a file, overwriting its previous content.
 * @param path The path to the file.
 * @param content The string content to write.
 * @return 0 on success, -1 on failure.
 */
int write_file_content(const char* path, const char* content) {
    FILE* file = fopen(path, "w");
    if (!file) return -1;
    fprintf(file, "%s", content);
    fclose(file);
    return 0;
}