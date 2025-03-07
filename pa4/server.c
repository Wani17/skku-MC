#include <rpc/rpc.h>
#include "who.h"
#include <stdio.h>
#include <stdlib.h>
#include <utmp.h>
#include <string.h>

void get_logged_in_users(data* ret) {
    struct utmp *entry;
    // static const size_t MAX_BUFFER_SIZE = 1000; // Fixed buffer size
    // char *result = (char *)malloc(MAX_BUFFER_SIZE);
    // if (!result) {
        // perror("Memory allocation failed");
        // return NULL;
    // }

    ret->result[0] = '\0'; // Initialize the string
    ret->size = 0; // Tracks the current buffer usage

    // Initialize reading from the utmp file
    setutent();

    // Read entries from the utmp file
    while ((entry = getutent()) != NULL) {
        if (entry->ut_type == USER_PROCESS) { // Only process logged-in users
            char line[256];
            snprintf(line, sizeof(line), "%-16s %-16s %-16s\n",
                     entry->ut_user,
                     entry->ut_line,
                     entry->ut_host[0] ? entry->ut_host : "Local");

            size_t line_length = strlen(line);
            if (ret->size + line_length >= MAXBUFFER) {
                // Stop if the buffer limit is exceeded
                fprintf(stderr, "Buffer limit exceeded: Cannot add more data.\n");
                break;
            }

            // Append the new line to the result string
            strcat(ret->result, line);
            ret->size += line_length;
        }
    }

    // Close the utmp file
    endutent();

    return;
}

data *remote_who_1_svc(void*, struct svc_req *svc) {
    static data ret;
    get_logged_in_users(&ret);
    
    printf("server will send %lu characters.\n", ret.size);
    return &ret; 
}


