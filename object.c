#include <openssl/sha.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "object.h"
#include "pes.h"

int object_write(ObjectType type, const void *data, size_t size, ObjectID *id) {
    if (!data || !id) return -1;

    SHA256((const unsigned char *)data, size, id->hash);

    char hex[HASH_HEX_SIZE + 1];
    hash_to_hex(id, hex);

    mkdir(".pes", 0755);
    mkdir(".pes/objects", 0755);

    char path[256];
    snprintf(path, sizeof(path), ".pes/objects/%s", hex);

    FILE *f = fopen(path, "wb");
    if (!f) return -1;

    fwrite(data, 1, size, f);
    fclose(f);

    return 0;
}

int object_read(const char *hash, void **data, size_t *size) {
    char path[256];
    snprintf(path, sizeof(path), ".pes/objects/%s", hash);

    FILE *f = fopen(path, "rb");
    if (!f) return -1;

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    rewind(f);

    *data = malloc(sz + 1);
    if (!*data) {
        fclose(f);
        return -1;
    }

    fread(*data, 1, sz, f);
    fclose(f);

    ((char *)(*data))[sz] = '\0';
    *size = sz;

    return 0;
}

void hash_to_hex(const ObjectID *id, char *hex_out) {
    for (int i = 0; i < HASH_SIZE; i++) {
        sprintf(hex_out + (2 * i), "%02x", id->hash[i]);
    }
    hex_out[HASH_HEX_SIZE] = '\0';
}

int hex_to_hash(const char *hex, ObjectID *id_out) {
    if (!hex || !id_out) return -1;

    for (int i = 0; i < HASH_SIZE; i++) {
        unsigned int byte;
        if (sscanf(hex + (2 * i), "%2x", &byte) != 1) {
            return -1;
        }
        id_out->hash[i] = (uint8_t)byte;
    }

    return 0;
}
// cleanup unused warnings
