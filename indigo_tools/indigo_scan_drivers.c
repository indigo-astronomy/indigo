//
//  indigo_scan_drivers.c
//
//  Scans directory for indigo drivers, collects information and unloads them
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <limits.h>
#include <sys/types.h>

#include <indigo/indigo_client.h>
#include <indigo/indigo_driver.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define MAX_DRIVERS 256

typedef struct {
    char name[INDIGO_NAME_SIZE];
    char description[INDIGO_NAME_SIZE];
    int version;
} driver_summary;

int main(int argc, char **argv) {
    DIR *dir;
    struct dirent *ent;
    char *directory = ".";
    driver_summary driver_list[MAX_DRIVERS];
    int driver_count = 0;
    
    if (argc > 1) {
        directory = argv[1];
    }
    
    printf("Scanning directory %s for indigo drivers...\n", directory);
    
    if ((dir = opendir(directory)) != NULL) {
        while ((ent = readdir(dir)) != NULL && driver_count < MAX_DRIVERS) {
            char *filename = ent->d_name;
            char fullpath[PATH_MAX];

            if (strncmp(filename, "indigo_", 7) == 0 && 
                strlen(filename) > 10 && 
                strcmp(filename + strlen(filename) - 3, ".so") == 0) {
                snprintf(fullpath, sizeof(fullpath), "%s/%s", directory, filename);

                indigo_driver_entry *driver;
                if (indigo_load_driver(fullpath, false, &driver) == INDIGO_OK) {
                    indigo_driver_info info;

                    driver->driver(INDIGO_DRIVER_INFO, &info);
                    
                    strncpy(driver_list[driver_count].name, info.name, INDIGO_NAME_SIZE);
                    strncpy(driver_list[driver_count].description, info.description, INDIGO_NAME_SIZE);
                    driver_list[driver_count].version = info.version;
                    driver_count++;
                    
                    indigo_remove_driver(driver);
                    // printf("Processed driver: %s\n", filename);
                } else {
                    printf("Failed to load driver: %s\n", fullpath);
                }
            }
        }
        closedir(dir);

        for (int i = 0; i < driver_count; i++) {
            printf("%s: %s\n", driver_list[i].name, driver_list[i].description);
        }
        printf("Total drivers found: %d\n", driver_count);
        
    } else {
        perror("Could not open directory");
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}
