#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "index.h"
#include "object.h"

#define INDEX_FILE ".pes/index"

int index_load(Index *index) {
    return 0;
}

int index_add(Index *index, const char *filename) {
    (void)index;

    FILE *f = fopen(filename, "rb");
    if (!f) return -1;

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    rewind(f);

    void *buffer = malloc(size);
    if (!buffer) {
        fclose(f);
        return -1;
    }

    fread(buffer, 1, size, f);
    fclose(f);

    ObjectID id;
    if (object_write(OBJ_BLOB, buffer, size, &id) != 0) {
        free(buffer);
        return -1;
    }

    free(buffer);

    char hash[HASH_HEX_SIZE + 1];
    hash_to_hex(&id, hash);

    FILE *idx = fopen(INDEX_FILE, "a");
    if (!idx) return -1;

    fprintf(idx, "%s %s\n", hash, filename);
    fclose(idx);

    return 0;
}

void index_status(void) {
    FILE *f = fopen(INDEX_FILE, "r");
    if (!f) {
        printf("No files staged\n");
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        printf("%s", line);
    }

    fclose(f);
}
