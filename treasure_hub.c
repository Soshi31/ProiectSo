#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/select.h>

#define CMD_FILE "hub_command.txt"
#define PIPE_NAME "monitor_pipe"

pid_t monitor_pid = -1;
int monitor_alive = 0;

void sigchld_handler(int sig) {
    int status;
    pid_t child_pid = waitpid(-1, &status, WNOHANG);
    if (child_pid == monitor_pid) {
        printf("Monitor exited with status %d\n", WEXITSTATUS(status));
        monitor_alive = 0;
    }
}

void start_monitor() {
    if (monitor_alive) {
        printf("Monitor is already running.\n");
        return;
    }

    unlink(PIPE_NAME);
    if (mkfifo(PIPE_NAME, 0666) == -1) {
        perror("mkfifo");
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

    int fd = open(CMD_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) { 
        perror("open"); 
        return; 
    }
    write(fd, cmd, strlen(cmd));
    close(fd);
    kill(monitor_pid, SIGUSR1);
    printf("[Hub] Sent SIGUSR1 to monitor\n");

    int pipe_fd = open(PIPE_NAME, O_RDONLY);
    if (pipe_fd < 0) { 
        perror("pipe read"); 
        return; 
    }

    char buf[512];
    int n;
    while ((n = read(pipe_fd, buf, sizeof(buf) - 1)) > 0) {
        buf[n] = '\0';
        printf("%s", buf);
    }
    close(pipe_fd);
}

void calculate_scores() {
    // Temporarily ignore SIGCHLD to prevent interference
    struct sigaction sa, old_sa;
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGCHLD, &sa, &old_sa);

    DIR *d = opendir(".");
    if (!d) { 
        perror("opendir"); 
        sigaction(SIGCHLD, &old_sa, NULL);
        return; 
    }

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (ent->d_type == DT_DIR &&
            strcmp(ent->d_name, ".") != 0 &&
            strcmp(ent->d_name, "..") != 0 &&
            strncmp(ent->d_name, "logged_hunt-", 12) != 0 &&
            strcmp(ent->d_name, ".git") != 0) {

            char treasure_path[512];
            snprintf(treasure_path, sizeof(treasure_path), "%s/treasures.dat", ent->d_name);
            if (access(treasure_path, F_OK) != 0) {
                continue;
            }

            int fd[2];
            if (pipe(fd) == -1) {
                perror("pipe");
                continue;
            }

            pid_t pid = fork();
            if (pid == 0) {
                close(fd[0]);
                dup2(fd[1], STDOUT_FILENO);
                close(fd[1]);
                
                execl("./score_calculator", "./score_calculator", ent->d_name, NULL);
                perror("exec score_calculator");
                _exit(1);
            } else if (pid > 0) {
                close(fd[1]);
                printf("Scores for %s:\n", ent->d_name);

                fd_set read_fds;
                struct timeval timeout;
                FD_ZERO(&read_fds);
                FD_SET(fd[0], &read_fds);

                timeout.tv_sec = 5;
                timeout.tv_usec = 0;

                int ready = select(fd[0] + 1, &read_fds, NULL, NULL, &timeout);
                if (ready > 0) {
                    char buf[256];
                    int n;
                    while ((n = read(fd[0], buf, sizeof(buf) - 1)) > 0) {
                        buf[n] = '\0';
                        printf("%s", buf);
                    }
                } else if (ready == 0) {
                    printf("(timeout reading scores for %s)\n", ent->d_name);
                    kill(pid, SIGKILL);
                } else {
                    perror("select error");
                    kill(pid, SIGKILL);
                }

                close(fd[0]);
                waitpid(pid, NULL, 0);
            } else {
                perror("fork");
                close(fd[0]);
                close(fd[1]);
            }
        }
    }
    closedir(d);
    sigaction(SIGCHLD, &old_sa, NULL);
}

int main() {
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGCHLD, &sa, NULL);

    char line[256];
    while (1) {
        printf("hub> ");
        if (!fgets(line, sizeof(line), stdin)) break;
        line[strcspn(line, "\n")] = 0;

        if (strcmp(line, "start_monitor") == 0) {
            start_monitor();
        }
        else if (strcmp(line, "stop_monitor") == 0) {
            if (!monitor_alive) {
                printf("Monitor is not running.\n");
            } else {
                kill(monitor_pid, SIGTERM);
            }
        }
        else if (strcmp(line, "exit") == 0) {
            if (monitor_alive) {
                printf("Error: monitor still running. Stop it first.\n");
            } else {
                break;
            }
        }
        else if (strcmp(line, "calculate_scores") == 0) {
            calculate_scores();
        }
        else if (strncmp(line, "list_hunts", 10) == 0 ||
                 strncmp(line, "list_treasures", 14) == 0 ||
                 strncmp(line, "view_treasure", 13) == 0) {
            send_command(line);
        }
        else {
            printf("Invalid command.\n");
        }
    }

    return 0;
}
