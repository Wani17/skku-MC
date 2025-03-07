#include "who.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
    CLIENT *client;
    data *result;

    if (argc != 2) {
        printf("Usage: %s <hostname>\n", argv[0]);
        return 1;
    }


    // Create RPC client
    client = clnt_create(argv[1], WHOPROG, WHOVERS, "udp");
    if (client == NULL) {
        clnt_pcreateerror(argv[1]);
        return 1;
    }

    // Call the remote procedure
    result = remote_who_1(NULL, client);
    if (result == NULL) {
        clnt_perror(client, argv[1]);
        return 1;
    }

    printf("%s\n", result->result);

    clnt_destroy(client);
    return 0;
}

