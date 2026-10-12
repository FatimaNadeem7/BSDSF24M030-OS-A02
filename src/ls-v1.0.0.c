#define _DEFAULT_SOURCE

#include <dirent.h>
#include <errno.h>
#include <grp.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

typedef struct {
    char **items;
    size_t count;
    size_t capacity;
} NameList;

static int flag_long = 0;
static int flag_across = 0;
static int flag_recursive = 0;
static int flag_color = 0;

static void add_name(NameList *list, const char *name)
{
    if (list->count == list->capacity) {
        size_t new_capacity = list->capacity ? list->capacity * 2 : 32;
        char **new_items = realloc(list->items,
                                   new_capacity * sizeof(char *));
        if (!new_items) {
            perror("realloc");
            exit(EXIT_FAILURE);
        }
        list->items = new_items;
        list->capacity = new_capacity;
    }

    list->items[list->count] = strdup(name);
    if (!list->items[list->count]) {
        perror("strdup");
        exit(EXIT_FAILURE);
    }

    list->count++;
}

static void free_names(NameList *list)
{
    for (size_t i = 0; i < list->count; i++)
        free(list->items[i]);

    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

static int compare_names(const void *a, const void *b)
{
    const char *name_a = *(const char * const *)a;
    const char *name_b = *(const char * const *)b;

    int result = strcasecmp(name_a, name_b);

    if (result == 0)
        result = strcmp(name_a, name_b);

    return result;
}

static char *join_path(const char *directory, const char *name)
{
    size_t dlen = strlen(directory);
    size_t nlen = strlen(name);
    int slash = dlen > 0 && directory[dlen - 1] != '/';

    char *result = malloc(dlen + (size_t)slash + nlen + 1);

    if (!result) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    snprintf(result, dlen + (size_t)slash + nlen + 1,
             "%s%s%s", directory, slash ? "/" : "", name);

    return result;
}

static int has_suffix(const char *name, const char *suffix)
{
    size_t nlen = strlen(name);
    size_t slen = strlen(suffix);

    return nlen >= slen &&
           strcasecmp(name + nlen - slen, suffix) == 0;
}

static int is_archive(const char *name)
{
    return has_suffix(name, ".tar") ||
           has_suffix(name, ".gz")  ||
           has_suffix(name, ".tgz") ||
           has_suffix(name, ".zip") ||
           has_suffix(name, ".bz2") ||
           has_suffix(name, ".xz")  ||
           has_suffix(name, ".7z");
}

/* Assignment color scheme:
   directories: blue
   executables: green
   archives: red
   symbolic links: pink
   special files: reverse video
*/
static const char *file_color(const char *path, const struct stat *st)
{
    if (!flag_color)
        return "";

    if (S_ISLNK(st->st_mode))
        return "\033[1;35m";

    if (S_ISDIR(st->st_mode))
        return "\033[1;34m";

    if (S_ISCHR(st->st_mode) || S_ISBLK(st->st_mode) ||
        S_ISFIFO(st->st_mode) || S_ISSOCK(st->st_mode))
        return "\033[7m";

    if (is_archive(path))
        return "\033[1;31m";

    if (st->st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))
        return "\033[1;32m";

    return "";
}

static void print_colored_name(const char *name, const char *path,
                               const struct stat *st)
{
    const char *color = file_color(path, st);

    if (color[0] != '\0')
        printf("%s", color);

    printf("%s", name);

    if (color[0] != '\0')
        printf("\033[0m");
}

static void print_permissions(mode_t mode, char output[11])
{
    output[0] = S_ISDIR(mode)  ? 'd' :
                S_ISLNK(mode)  ? 'l' :
                S_ISCHR(mode)  ? 'c' :
                S_ISBLK(mode)  ? 'b' :
                S_ISFIFO(mode) ? 'p' :
                S_ISSOCK(mode) ? 's' : '-';

    output[1] = mode & S_IRUSR ? 'r' : '-';
    output[2] = mode & S_IWUSR ? 'w' : '-';
    output[3] = mode & S_IXUSR ? 'x' : '-';
    output[4] = mode & S_IRGRP ? 'r' : '-';
    output[5] = mode & S_IWGRP ? 'w' : '-';
    output[6] = mode & S_IXGRP ? 'x' : '-';
    output[7] = mode & S_IROTH ? 'r' : '-';
    output[8] = mode & S_IWOTH ? 'w' : '-';
    output[9] = mode & S_IXOTH ? 'x' : '-';

    if (mode & S_ISUID)
        output[3] = (mode & S_IXUSR) ? 's' : 'S';

    if (mode & S_ISGID)
        output[6] = (mode & S_IXGRP) ? 's' : 'S';

    if (mode & S_ISVTX)
        output[9] = (mode & S_IXOTH) ? 't' : 'T';

    output[10] = '\0';
}

static void print_long_entry(const char *name, const char *path,
                             const struct stat *st)
{
    char permissions[11];
    char date_buffer[64];
    char link_target[PATH_MAX + 1];

    print_permissions(st->st_mode, permissions);

    struct passwd *user = getpwuid(st->st_uid);
    struct group *group = getgrgid(st->st_gid);

    time_t file_time = st->st_mtime;
    struct tm *time_info = localtime(&file_time);

    if (time_info)
        strftime(date_buffer, sizeof(date_buffer),
                 "%b %d %H:%M", time_info);
    else
        snprintf(date_buffer, sizeof(date_buffer), "??? ?? ??:??");

    printf("%s %2lu %-8s %-8s %8lld %s ",
           permissions,
           (unsigned long)st->st_nlink,
           user ? user->pw_name : "unknown",
           group ? group->gr_name : "unknown",
           (long long)st->st_size,
           date_buffer);

    print_colored_name(name, path, st);

    if (S_ISLNK(st->st_mode)) {
        ssize_t length = readlink(path, link_target, PATH_MAX);

        if (length >= 0) {
            link_target[length] = '\0';
            printf(" -> %s", link_target);
        }
    }

    putchar('\n');
}

static int terminal_width(void)
{
    struct winsize window;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &window) == 0 &&
        window.ws_col > 0)
        return window.ws_col;

    return 80;
}

static void display_columns(const char *directory, NameList *names)
{
    if (names->count == 0)
        return;

    size_t max_length = 0;

    for (size_t i = 0; i < names->count; i++) {
        size_t length = strlen(names->items[i]);
        if (length > max_length)
            max_length = length;
    }

    size_t cell_width = max_length + 2;
    if (cell_width == 0)
        cell_width = 1;

    size_t columns = (size_t)terminal_width() / cell_width;
    if (columns < 1)
        columns = 1;

    if (columns > names->count)
        columns = names->count;

    size_t rows = (names->count + columns - 1) / columns;

    for (size_t r = 0; r < rows; r++) {
        for (size_t c = 0; c < columns; c++) {
            size_t index;

            if (flag_across)
                index = r * columns + c;
            else
                index = c * rows + r;

            if (index >= names->count)
                continue;

            const char *name = names->items[index];
            char *path = join_path(directory, name);
            struct stat st;

            if (lstat(path, &st) == -1) {
                free(path);
                continue;
            }

            print_colored_name(name, path, &st);

            if (c + 1 < columns) {
                size_t padding = cell_width - strlen(name);
                for (size_t p = 0; p < padding; p++)
                    putchar(' ');
            }

            free(path);
        }

        putchar('\n');
    }
}

static int is_dot_directory(const char *name)
{
    return strcmp(name, ".") == 0 || strcmp(name, "..") == 0;
}

static void list_directory(const char *directory, int show_header)
{
    DIR *dir = opendir(directory);

    if (!dir) {
        fprintf(stderr, "ls: cannot open '%s': %s\n",
                directory, strerror(errno));
        return;
    }

    if (show_header)
        printf("%s:\n", directory);

    NameList names = {0};
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        if (!is_dot_directory(entry->d_name))
            add_name(&names, entry->d_name);
    }

    closedir(dir);

    qsort(names.items, names.count, sizeof(char *), compare_names);

    if (flag_long) {
        for (size_t i = 0; i < names.count; i++) {
            char *path = join_path(directory, names.items[i]);
            struct stat st;

            if (lstat(path, &st) == -1) {
                fprintf(stderr, "ls: cannot access '%s': %s\n",
                        path, strerror(errno));
            } else {
                print_long_entry(names.items[i], path, &st);
            }

            free(path);
        }
    } else {
        display_columns(directory, &names);
    }

    if (flag_recursive) {
        for (size_t i = 0; i < names.count; i++) {
            char *path = join_path(directory, names.items[i]);
            struct stat st;

            if (lstat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
                putchar('\n');
                list_directory(path, 1);
            }

            free(path);
        }
    }

    free_names(&names);
}

static void print_usage(const char *program)
{
    fprintf(stderr,
            "Usage: %s [-l] [-x] [-R] [--color] [directory ...]\n",
            program);
}

int main(int argc, char *argv[])
{
    char **paths = calloc((size_t)argc + 1, sizeof(char *));

    if (!paths) {
        perror("calloc");
        return EXIT_FAILURE;
    }

    int path_count = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--color") == 0) {
            flag_color = 1;
        } else if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            free(paths);
            return EXIT_SUCCESS;
        } else if (argv[i][0] == '-' && argv[i][1] != '\0') {
            for (size_t j = 1; argv[i][j] != '\0'; j++) {
                switch (argv[i][j]) {
                    case 'l':
                        flag_long = 1;
                        break;
                    case 'x':
                        flag_across = 1;
                        break;
                    case 'R':
                        flag_recursive = 1;
                        break;
                    default:
                        fprintf(stderr, "ls: invalid option -- '%c'\n",
                                argv[i][j]);
                        print_usage(argv[0]);
                        free(paths);
                        return EXIT_FAILURE;
                }
            }
        } else {
            paths[path_count++] = argv[i];
        }
    }

    if (path_count == 0)
        paths[path_count++] = ".";

    for (int i = 0; i < path_count; i++) {
        struct stat st;

        if (lstat(paths[i], &st) == -1) {
            fprintf(stderr, "ls: cannot access '%s': %s\n",
                    paths[i], strerror(errno));
            continue;
        }

        if (!S_ISDIR(st.st_mode)) {
            if (flag_long) {
                print_long_entry(paths[i], paths[i], &st);
            } else {
                print_colored_name(paths[i], paths[i], &st);
                putchar('\n');
            }
        } else {
            int show_header = flag_recursive || path_count > 1;

            if (i > 0)
                putchar('\n');

            list_directory(paths[i], show_header);
        }
    }

    free(paths);
    return EXIT_SUCCESS;
}
