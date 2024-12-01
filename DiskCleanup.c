#include <windows.h>
#include <shlobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_PATH_LENGTH 260

// Logging function
void log_action(const char *action, const char *details) {
    FILE *logFile = fopen("cleanup_log.txt", "a");
    if (logFile) {
        time_t now = time(NULL);
        char timeStr[20];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
        fprintf(logFile, "[%s] %s: %s", timeStr, action, details);
        fclose(logFile);
    }
}

// Secure deletion function
void secure_delete(const char *filePath) {
    FILE *file = fopen(filePath, "rb+");
    if (file) {
        fseek(file, 0, SEEK_END);
        long fileSize = ftell(file);
        rewind(file);

        char *buffer = (char *)malloc(fileSize);
        if (buffer) {
            memset(buffer, 0, fileSize);
            fwrite(buffer, 1, fileSize, file);
            free(buffer);
        }
        fclose(file);
        DeleteFile(filePath);
        log_action("Securely deleted", filePath);
    } else {
        log_action("Failed to securely delete", filePath);
    }
}

// Cleanup files by size
void cleanup_by_size(const char *dir, long sizeThreshold, int simulate) {
    WIN32_FIND_DATA findFileData;
    HANDLE hFind;
    char searchPath[MAX_PATH_LENGTH];
    sprintf(searchPath, "%s\\*.*", dir);
    hFind = FindFirstFile(searchPath, &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            char filePath[MAX_PATH_LENGTH];
            sprintf(filePath, "%s\%s", dir, findFileData.cFileName);
            
            LARGE_INTEGER fileSize;
            fileSize.LowPart = findFileData.nFileSizeLow;
            fileSize.HighPart = findFileData.nFileSizeHigh;
            
            if (fileSize.QuadPart > sizeThreshold) {
                if (simulate) {
                    printf("Simulated deletion: %s (%lld bytes)", filePath, fileSize.QuadPart);
                    log_action("Simulated deletion", filePath);
                } else {
                    if (DeleteFile(filePath)) {
                        printf("Deleted: %s (%lld bytes)", filePath, fileSize.QuadPart);
                        log_action("Deleted", filePath);
                    } else {
                        log_action("Failed to delete", filePath);
                    }
                }
            }
        }
    } while (FindNextFile(hFind, &findFileData));
    FindClose(hFind);
}

// Cleanup files by modification date
void cleanup_by_date(const char *dir, int daysThreshold, int simulate) {
    WIN32_FIND_DATA findFileData;
    HANDLE hFind;
    char searchPath[MAX_PATH_LENGTH];
    sprintf(searchPath, "%s\\*.*", dir);
    hFind = FindFirstFile(searchPath, &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) return;

    time_t currentTime = time(NULL);
    do {
        if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            char filePath[MAX_PATH_LENGTH];
            sprintf(filePath, "%s\%s", dir, findFileData.cFileName);

            FILETIME ft = findFileData.ftLastWriteTime;
            SYSTEMTIME stUTC, stLocal;
            FileTimeToSystemTime(&ft, &stUTC);
            SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);

            struct tm lastModified;
            lastModified.tm_year = stLocal.wYear - 1900;
            lastModified.tm_mon = stLocal.wMonth - 1;
            lastModified.tm_mday = stLocal.wDay;
            lastModified.tm_hour = stLocal.wHour;
            lastModified.tm_min = stLocal.wMinute;
            lastModified.tm_sec = stLocal.wSecond;
            lastModified.tm_isdst = -1;
            time_t fileTime = mktime(&lastModified);

            double daysDiff = difftime(currentTime, fileTime) / (60 * 60 * 24);
            if (daysDiff > daysThreshold) {
                if (simulate) {
                    printf("Simulated deletion: %s (Last Modified: %d days ago)", filePath, (int)daysDiff);
                    log_action("Simulated deletion by date", filePath);
                } else {
                    if (DeleteFile(filePath)) {
                        printf("Deleted: %s (Last Modified: %d days ago)", filePath, (int)daysDiff);
                        log_action("Deleted by date", filePath);
                    } else {
                        log_action("Failed to delete by date", filePath);
                    }
                }
            }
        }
    } while (FindNextFile(hFind, &findFileData));
    FindClose(hFind);
}

// Menu and main program
void show_menu() {
    printf("Disk Cleanup Utility - Advanced\n");
    printf("================================\n");
    printf("1. Clean files larger than a size\n");
    printf("2. Clean files older than a date\n");
    printf("3. Secure delete a file\n");
    printf("4. Simulate cleanup\n");
    printf("5. Exit\n");
    printf("Choose an option: ");
}

int main() {
    int choice;
    char dir[MAX_PATH_LENGTH];
    while (1) {
        show_menu();
        scanf("%d", &choice);
        getchar();  // Consume newline

        switch (choice) {
            case 1: {
                printf("Enter directory path: ");
                fgets(dir, MAX_PATH_LENGTH, stdin);
                dir[strcspn(dir, "\n")] = 0;  // Remove newline
                long sizeThreshold;
                printf("Enter size threshold in bytes: ");
                scanf("%ld", &sizeThreshold);
                cleanup_by_size(dir, sizeThreshold, 0);
                break;
            }
            case 2: {
                printf("Enter directory path: ");
                fgets(dir, MAX_PATH_LENGTH, stdin);
                dir[strcspn(dir, "\n")] = 0;  // Remove newline
                int daysThreshold;
                printf("Enter days threshold: ");
                scanf("%d", &daysThreshold);
                cleanup_by_date(dir, daysThreshold, 0);
                break;
            }
            case 3: {
                printf("Enter file path for secure delete: ");
                fgets(dir, MAX_PATH_LENGTH, stdin);
                dir[strcspn(dir, "\n")] = 0;  // Remove newline
                secure_delete(dir);
                break;
            }
            case 4: {
                printf("Enter directory path: ");
                fgets(dir, MAX_PATH_LENGTH, stdin);
                dir[strcspn(dir, "\n")] = 0;  // Remove newline
                long sizeThreshold;
                printf("Enter size threshold in bytes (simulation): ");
                scanf("%ld", &sizeThreshold);
                cleanup_by_size(dir, sizeThreshold, 1);
                break;
            }
            case 5:
                printf("Exiting program.");
                return 0;
            default:
                printf("Invalid option. Try again.");
                break;
        }
    }
    return 0;
}