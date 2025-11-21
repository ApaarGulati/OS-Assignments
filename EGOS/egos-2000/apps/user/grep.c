#include "app.h"
#include <string.h>
#include <stdint.h>

#define MAX_FILE_SIZE 65536  // 64 KB max file size
#define PRINT_CHUNK 512      // Print in 512-byte chunks

int main(int argc, char **argv) {
    // Check arguments
    if (argc != 3) {
        INFO("usage: grep [PATTERN] [FILE]");
        return -1;
    }

    // Pattern to search for in filename
    char *pattern = argv[1];
    char *filename = argv[2];

    // Search for the file in directory system
    int file_ino = dir_lookup(workdir_ino, filename);  // Unique identifier
    if (file_ino < 0) {
        INFO("grep: file %s not found", filename);
        return -1;
    }

    char buf[BLOCK_SIZE];              // Buffer to read one block at a time
    char file_buf[MAX_FILE_SIZE];      // Buffer to store the entire file
    int file_len = 0;
    int block = 0;

    // Read the entire file
    while (1) {
        // Read one block at a time
        int ret = file_read(file_ino, block, buf);
        if (ret < 0) break;  // EOF

        // Copy characters until first zero or end of block into file buffer
        int buf_len = 0;
        for (int i = 0; i < BLOCK_SIZE; i++) {
            if (buf[i] == '\0') break;  // stop at zero padding
            if (file_len + buf_len >= MAX_FILE_SIZE) {
                INFO("File too large to process");
                return -1;
            }
            file_buf[file_len + buf_len] = buf[i];
            buf_len++;
        }
        file_len += buf_len;
        block++;
    }

    // Process lines
    int line_start = 0;
    for (int i = 0; i <= file_len; i++) {
        // Finding line end
        if (file_buf[i] == '\n' || i == file_len) {
            // Line length
            int line_len = i - line_start;

            // Skip empty lines
            if (line_len <= 0) {
                line_start = i + 1;
                continue;
            }

            // Temporarily null-terminate the line (to make strstr work)
            file_buf[line_start + line_len] = '\0';

            // Use strstr to check if pattern exists
            if (strstr(&file_buf[line_start], pattern) != NULL) {
                int remaining = line_len;           // Remaining length to print
                int offset = 0;                     // Where in the line to start printing from
                char print_buf[PRINT_CHUNK];        // Temporary buffer for printing

                while (remaining > 0) {
                    // Find chunk size to be printed
                    int chunk = remaining;
                    if (chunk > PRINT_CHUNK - 1) {
                        chunk = PRINT_CHUNK - 1;
                    }
                    
                    // Copy the chunk to print buffer
                    for (int j = 0; j < chunk; j++) {
                        print_buf[j] = file_buf[line_start + offset + j];
                    }
                    print_buf[chunk] = '\0';

                    my_printf("%s", print_buf);
                    
                    remaining -= chunk;
                    offset += chunk;
                }
                my_printf("\n");  // final newline
            }

            line_start = i + 1;
        }
    }

    return 0;
}
