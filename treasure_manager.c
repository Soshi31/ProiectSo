#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CLAUSE_LENGTH 256
#define MAX_USERNAME_LENGTH 50
#define FILENAME "treasure_data.txt"

typedef struct {
    int id;
    char username[MAX_USERNAME_LENGTH];
    float latitude;
    float longitude;
    char clue[MAX_CLAUSE_LENGTH];
    int value;
} Treasure;

void saveTreasureToFile(Treasure *treasure) {
    FILE *file = fopen(FILENAME, "a");
    if (file == NULL) {
        perror("Unable to open file for writing");
        exit(1);
    }

    fprintf(file, "%d,%s,%f,%f,%s,%d\n",
            treasure->id,
            treasure->username,
            treasure->latitude,
            treasure->longitude,
            treasure->clue,
            treasure->value);

    fclose(file);
}

void readTreasuresFromFile() {
    FILE *file = fopen(FILENAME, "r");
    if (file == NULL) {
        perror("Unable to open file for reading");
        exit(1);
    }

    Treasure treasure;
    printf("Treasures in the system:\n");
    printf("ID\tUsername\tLatitude\tLongitude\tClue\t\t\t\tValue\n");
    printf("--------------------------------------------------------------------------\n");

    while (fscanf(file, "%d,%49[^,],%f,%f,%255[^,],%d\n",
                  &treasure.id, treasure.username, &treasure.latitude, &treasure.longitude, treasure.clue, &treasure.value) != EOF) {
        printf("%d\t%s\t%f\t%f\t%-25s\t%d\n", 
               treasure.id, treasure.username, treasure.latitude, treasure.longitude, treasure.clue, treasure.value);
    }

    fclose(file);
}

void getUserInput(Treasure *treasure) {
    printf("Enter Treasure ID: ");
    scanf("%d", &treasure->id);
    getchar();  

    printf("Enter Username: ");
    fgets(treasure->username, MAX_USERNAME_LENGTH, stdin);
    treasure->username[strcspn(treasure->username, "\n")] = '\0';

    printf("Enter Latitude: ");
    scanf("%f", &treasure->latitude);

    printf("Enter Longitude: ");
    scanf("%f", &treasure->longitude);
    getchar();

    printf("Enter Clue: ");
    fgets(treasure->clue, MAX_CLAUSE_LENGTH, stdin);
    treasure->clue[strcspn(treasure->clue, "\n")] = '\0'; 

    printf("Enter Treasure Value: ");
    scanf("%d", &treasure->value);
}

int main() {
    int choice;
    Treasure treasure;

    while (1) {
        printf("\nTreasure Manager\n");
        printf("1. Add a New Treasure\n");
        printf("2. View All Treasures\n");
        printf("3. Exit\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                getUserInput(&treasure);
                saveTreasureToFile(&treasure);
                printf("Treasure has been added!\n");
                break;
            case 2:
                readTreasuresFromFile();
                break;
            case 3:
                printf("Exiting program.\n");
                return 0;
            default:
                printf("Invalid choice. Please try again.\n");
        }
    }

    return 0;
}
