#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define CMD_FILE "hub_command.txt"

pid_t monitor_pid = -1;
int   monitor_alive = 0;

void sigchld_handler(int sig) {
    int status;
    waitpid(monitor_pid, &status, 0);
    printf("Monitor exited with status %d\n", WEXITSTATUS(status));
    monitor_alive = 0;
}

void start_monitor() {
    if (monitor_alive) {
        printf("Monitor is already running.\n");
        return;
    }
    monitor_pid = fork();
    if (monitor_pid == 0) {
        execl("./monitor", "./monitor", NULL);
        perror("execl");
        exit(1);
    } else if (monitor_pid > 0) {
        monitor_alive = 1;
        printf("Monitor started (PID: %d).\n", monitor_pid);
    } else {
        perror("fork");
    }
}

void send_command(const char *cmd) {
    if (!monitor_alive) {
        printf("Monitor is not running.\n");
        return;
    }
    int fd = open(CMD_FILE, O_WRONLY|O_CREAT|O_TRUNC, 0644);
    if (fd < 0) { perror("open"); return; }
    write(fd, cmd, strlen(cmd));
    close(fd);
    kill(monitor_pid, SIGUSR1);
}

int main() {
    signal(SIGCHLD, sigchld_handler);

    char line[256];
    while (1) {
        printf("hub> ");
        if (!fgets(line, sizeof(line), stdin)) break;
        line[strcspn(line, "\n")] = 0;
        if (strcmp(line, "start_monitor")==0) {
            start_monitor();
        }
        else if (strcmp(line, "stop_monitor")==0) {
            if (!monitor_alive) {
                printf("Monitor is not running.\n");
            } else {
                kill(monitor_pid, SIGTERM);
            }
        }
        else if (strcmp(line, "exit")==0) {
            if (monitor_alive) {
                printf("Error: monitor still running. Stop it first.\n");
            } else {
                break;
            }
        }
        else if (strncmp(line, "list_hunts", 10)==0 ||
                 strncmp(line, "list_treasures", 14)==0 ||
                 strncmp(line, "view_treasure", 13)==0)
        {
            send_command(line);
        }
        else {
            printf("Invalid command.\n");
        }
    }
    return 0;
}
