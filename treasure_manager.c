#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>

#define MAX_USERNAME_LEN 32
#define MAX_CLUE_LEN 256
#define TREASURE_FILE "treasures.dat"
#define LOG_FILE "logged_hunt"
#define SYMLINK_PREFIX "logged_hunt-"

typedef struct {
    int id;
    char username[MAX_USERNAME_LEN];
    float latitude;
    float longitude;
    char clue[MAX_CLUE_LEN];
    int value;
} Treasure;

void log_operation(const char *hunt_id, const char *message) {
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", hunt_id, LOG_FILE);

    int fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) {
        perror("log open");
        return;
    }
    dprintf(fd, "%s\n", message);
    close(fd);
}

void create_symlink(const char *hunt_id) {
    char target[512], linkname[512];
    snprintf(target, sizeof(target), "%s/%s", hunt_id, LOG_FILE);
    snprintf(linkname, sizeof(linkname), "%s%s", SYMLINK_PREFIX, hunt_id);
    /* remove old symlink if exists */
    unlink(linkname);
    symlink(target, linkname);
}

void add_treasure(const char *hunt_id) {
    mkdir(hunt_id, 0755);

    char path[512];
    snprintf(path, sizeof(path), "%s/%s", hunt_id, TREASURE_FILE);

    Treasure t;
    printf("Treasure ID: ");        scanf("%d", &t.id);
    printf("Username: ");           scanf("%31s", t.username);
    printf("Latitude: ");           scanf("%f", &t.latitude);
    printf("Longitude: ");          scanf("%f", &t.longitude);
    printf("Clue: ");               getchar(); fgets(t.clue, MAX_CLUE_LEN, stdin);
                                    t.clue[strcspn(t.clue, "\n")] = 0;
    printf("Value: ");              scanf("%d", &t.value);

    int fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) { perror("add open"); return; }
    write(fd, &t, sizeof(t));
    close(fd);

    char msg[128];
    snprintf(msg, sizeof(msg), "Added treasure ID %d", t.id);
    log_operation(hunt_id, msg);
    create_symlink(hunt_id);
}

void list_treasures(const char *hunt_id) {
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", hunt_id, TREASURE_FILE);

    struct stat st;
    if (stat(path, &st) < 0) {
        perror("stat");
        return;
    }
    printf("Hunt: %s\nSize: %ld bytes\nLast modified: %s",
           hunt_id, st.st_size, ctime(&st.st_mtime));

    int fd = open(path, O_RDONLY);
    if (fd < 0) { perror("open"); return; }

    Treasure t;
    while (read(fd, &t, sizeof(t)) == sizeof(t)) {
        printf("ID:%d | User:%s | (%.4f,%.4f) | Value:%d | Clue:%s\n",
               t.id, t.username, t.latitude, t.longitude, t.value, t.clue);
    }
    close(fd);
}

void view_treasure(const char *hunt_id, int target_id) {
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", hunt_id, TREASURE_FILE);

    int fd = open(path, O_RDONLY);
    if (fd < 0) { perror("open"); return; }

    Treasure t;
    while (read(fd, &t, sizeof(t)) == sizeof(t)) {
        if (t.id == target_id) {
            printf("Treasure found:\n"
                   "ID: %d\nUser: %s\nCoords: (%.4f, %.4f)\n"
                   "Clue: %s\nValue: %d\n",
                   t.id, t.username, t.latitude, t.longitude, t.clue, t.value);
            close(fd);
            return;
        }
    }
    printf("Treasure ID %d not found.\n", target_id);
    close(fd);
}

void remove_treasure(const char *hunt_id, int target_id) {
    char path[512], tmp[512];
    snprintf(path, sizeof(path), "%s/%s", hunt_id, TREASURE_FILE);
    snprintf(tmp,  sizeof(tmp),  "%s/tmp.dat", hunt_id);

    int fd = open(path, O_RDONLY);
    int fd2= open(tmp, O_WRONLY|O_CREAT|O_TRUNC, 0644);
    if (fd<0||fd2<0){ perror("open"); return; }

    Treasure t;
    int found=0;
    while (read(fd,&t,sizeof(t))==sizeof(t)) {
        if (t.id==target_id) { found=1; continue; }
        write(fd2,&t,sizeof(t));
    }
    close(fd); close(fd2);
    rename(tmp,path);

    if (found) {
        char msg[128];
        snprintf(msg,sizeof(msg),"Removed treasure ID %d",target_id);
        log_operation(hunt_id,msg);
    } else {
        printf("Treasure ID %d not found.\n", target_id);
    }
}

void remove_hunt(const char *hunt_id) {
    char path[512], link[512];
    snprintf(path, sizeof(path), "%s/%s", hunt_id, TREASURE_FILE);
    unlink(path);
    snprintf(path, sizeof(path), "%s/%s", hunt_id, LOG_FILE);
    unlink(path);
    snprintf(link, sizeof(link), "%s%s", SYMLINK_PREFIX, hunt_id);
    unlink(link);
    rmdir(hunt_id);
    printf("Hunt %s removed.\n", hunt_id);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr,
          "Usage: %s --<add|list|view|remove_treasure|remove_hunt> <hunt_id> [<id>]\n",
          argv[0]);
        return 1;
    }

    if      (strcmp(argv[1],"--add")==0)            add_treasure(argv[2]);
    else if (strcmp(argv[1],"--list")==0)           list_treasures(argv[2]);
    else if (strcmp(argv[1],"--view")==0 && argc==4)    view_treasure(argv[2],atoi(argv[3]));
    else if (strcmp(argv[1],"--remove_treasure")==0 && argc==4)
                                                     remove_treasure(argv[2],atoi(argv[3]));
    else if (strcmp(argv[1],"--remove_hunt")==0)    remove_hunt(argv[2]);
    else {
        fprintf(stderr, "Invalid command.\n");
        return 1;
    }
    return 0;
}
