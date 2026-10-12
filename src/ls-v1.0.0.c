#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

void print_permissions(mode_t mode);
void print_long_format(const char *path, const char *name);
void do_ls(const char *dir, int long_format);

void print_permissions(mode_t mode)
{
    char p[11] = "----------";
    if (S_ISDIR(mode)) p[0] = 'd';
    else if (S_ISLNK(mode)) p[0] = 'l';
    if (mode & S_IRUSR) p[1] = 'r';
    if (mode & S_IWUSR) p[2] = 'w';
    if (mode & S_IXUSR) p[3] = 'x';
    if (mode & S_IRGRP) p[4] = 'r';
    if (mode & S_IWGRP) p[5] = 'w';
    if (mode & S_IXGRP) p[6] = 'x';
    if (mode & S_IROTH) p[7] = 'r';
    if (mode & S_IWOTH) p[8] = 'w';
    if (mode & S_IXOTH) p[9] = 'x';
    printf("%s", p);
}

void print_long_format(const char *path, const char *name)
{
    struct stat st;
    if (stat(path, &st) == -1) {
        perror(path);
        return;
    }

    struct passwd *pw = getpwuid(st.st_uid);
    struct group *gr = getgrgid(st.st_gid);
    char time_buffer[80];
    struct tm *time_info = localtime(&st.st_mtime);

    if (time_info != NULL)
        strftime(time_buffer, sizeof(time_buffer), "%b %d %H:%M", time_info);
    else
        strcpy(time_buffer, "unknown");

    print_permissions(st.st_mode);
    printf(" %lu %-8s %-8s %8ld %s %s\n",
           (unsigned long)st.st_nlink,
           pw ? pw->pw_name : "?",
           gr ? gr->gr_name : "?",
           (long)st.st_size, time_buffer, name);
}

void do_ls(const char *dir, int long_format)
{
    struct dirent *entry;
    DIR *dp = opendir(dir);

    if (dp == NULL) {
        perror(dir);
        return;
    }

    errno = 0;
    while ((entry = readdir(dp)) != NULL) {
        if (entry->d_name[0] == '.')
            continue;

        if (long_format) {
            char path[4096];
            snprintf(path, sizeof(path), "%s/%s", dir, entry->d_name);
            print_long_format(path, entry->d_name);
        } else {
            printf("%s\n", entry->d_name);
        }
    }

    if (errno != 0)
        perror("readdir");

    closedir(dp);
}

int main(int argc, char *argv[])
{
    int long_format = 0;
    int first_path = 1;

    if (argc > 1 && strcmp(argv[1], "-l") == 0) {
        long_format = 1;
        first_path = 2;
    }

    if (first_path >= argc) {
        do_ls(".", long_format);
    } else {
        for (int i = first_path; i < argc; i++) {
            if (argc - first_path > 1)
                printf("%s:\n", argv[i]);
            do_ls(argv[i], long_format);
            if (i < argc - 1)
                putchar('\n');
        }
    }

    return 0;
}
