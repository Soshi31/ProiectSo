#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define CMD_FILE "hub_command.txt"

volatile sig_atomic_t got_cmd = 0;

void handle_usr1(int sig) {
    got_cmd = 1;
}

void handle_term(int sig) {
    printf("Monitor shutting down...\n");
    usleep(1000000);
    exit(0);
}

void list_hunts() {
    DIR *d = opendir(".");
    if (!d) { perror("opendir"); return; }

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (ent->d_type == DT_DIR &&
            strcmp(ent->d_name, ".")!=0 &&
            strcmp(ent->d_name, "..")!=0 &&
            strncmp(ent->d_name, "logged_hunt-", 12)!=0)
        {
            /* count treasures in ent->d_name/treasures.dat */
            char path[256];
            snprintf(path, sizeof(path), "%s/treasures.dat", ent->d_name);

            FILE *f = fopen(path, "rb");
            int count = 0;
            if (f) {
                struct { int id; char u[32]; float la,lo; char c[256]; int v; } t;
                while (fread(&t, sizeof(t), 1, f)==1) count++;
                fclose(f);
            }
            printf("Hunt: %s | Total treasures: %d\n", ent->d_name, count);
        }
    }
    closedir(d);
}

void run_manager(int argc, char **argv) {
    pid_t pid = fork();
    if (pid == 0) {
        execv("./treasure_manager", argv);
        perror("execv");
        exit(1);
    } else if (pid > 0) {
        wait(NULL);
    } else {
        perror("fork");
    }
}

void process_cmd() {
    char buf[256];
    FILE *f = fopen(CMD_FILE, "r");
    if (!f) { perror("fopen"); return; }
    if (!fgets(buf, sizeof(buf), f)) { fclose(f); return; }
    fclose(f);
    buf[strcspn(buf, "\n")] = 0;

    char *tok = strtok(buf, " ");
    if (!tok) return;

    if (strcmp(tok, "list_hunts")==0) {
        list_hunts();
    }
    else if (strcmp(tok, "list_treasures")==0) {
        char *hunt = strtok(NULL, " ");
        if (hunt) {
            char *args[] = {"./treasure_manager", "--list", hunt, NULL};
            run_manager(3, args);
        } else {
            fprintf(stderr, "Usage: list_treasures <hunt_id>\n");
        }
    }
    else if (strcmp(tok, "view_treasure")==0) {
        char *hunt = strtok(NULL, " ");
        char *id   = strtok(NULL, " ");
        if (hunt && id) {
            char *args[] = {"./treasure_manager", "--view", hunt, id, NULL};
            run_manager(4, args);
        } else {
            fprintf(stderr, "Usage: view_treasure <hunt_id> <id>\n");
        }
    }
    else {
        fprintf(stderr, "Unknown command: %s\n", tok);
    }
}

int main() {
    struct sigaction sa1 = { .sa_handler = handle_usr1, .sa_flags = SA_RESTART };
    sigaction(SIGUSR1, &sa1, NULL);

    struct sigaction sa2 = { .sa_handler = handle_term, .sa_flags = SA_RESTART };
    sigaction(SIGTERM, &sa2, NULL);

    printf("Monitor ready (PID %d)\n", getpid());

    while (1) {
        pause();
        if (got_cmd) {
            process_cmd();
            got_cmd = 0;
        }
    }
    return 0;
}
