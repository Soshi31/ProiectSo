\
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int id;
    char username[32];
    float lat, lon;
    char clue[256];
    int value;
} Treasure;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <hunt_dir>\n", argv[0]);
        return 1;
    }

    char path[512];
    snprintf(path, sizeof(path), "%s/treasures.dat", argv[1]);

    FILE *f = fopen(path, "rb");
    if (!f) {
        perror("fopen");
        return 1;
    }

    Treasure t;
    struct Score {
        char user[32];
        int total;
    } scores[100];
    int sc = 0;

    while (fread(&t, sizeof(t), 1, f) == 1) {
        int found = 0;
        for (int i = 0; i < sc; ++i) {
            if (strcmp(scores[i].user, t.username) == 0) {
                scores[i].total += t.value;
                found = 1;
                break;
            }
        }
        if (!found && sc < 100) {
            strcpy(scores[sc].user, t.username);
            scores[sc].total = t.value;
            sc++;
        }
    }
    fclose(f);

    for (int i = 0; i < sc; ++i) {
        printf("%s: %d\n", scores[i].user, scores[i].total);
    }

    fflush(stdout);
    return 0;
}
