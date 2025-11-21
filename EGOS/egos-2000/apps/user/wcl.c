#include "app.h"
#include <string.h>

#define MAX_FILE_SIZE 65536  // 64 KB max file size

int main(int argc, char **argv) {
    // Check arguments
    if (argc < 2) {
        INFO("usage: wcl [FILE1] [FILE2] ...");
        return -1;
    }

    int total_lines = 0;

    // Iterate through the files
    for (int f = 1; f < argc; f++) {
        char *filename = argv[f];
        int file_ino = dir_lookup(workdir_ino, filename);
        if (file_ino < 0) {
            INFO("wcl: file %s not found", filename);
            return -1;
        }

        char buf[BLOCK_SIZE];              // Temporary block buffer
        char file_buf[MAX_FILE_SIZE];      // Full file buffer
        int file_len = 0;                  // Track total bytes read
        int block = 0;

        // Read entire file into file_buf
        while (1) {
            int ret = file_read(file_ino, block, buf);
            if (ret < 0) break;           // EOF or error
            block++;

            // Copy characters until first zero or end of block into file buffer
            for (int i = 0; i < BLOCK_SIZE; i++) {
                if (buf[i] == '\0') break;  // zero-padded end
                if (file_len >= MAX_FILE_SIZE) {
                    INFO("wcl: file %s too large to process", filename);
                    return -1;
                }
                file_buf[file_len++] = buf[i];
            }
        }

        // Count '\n' in the full file buffer
        for (int i = 0; i < file_len; i++) {
            if (file_buf[i] == '\n') {
                total_lines++;
            }
        }
    }

    // Print total line count
    printf("%d\n", total_lines);
    return 0;
}
