#include "tree.h"
#include "object.h"
#include "index.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int compare_entries(const void *a, const void *b) {
    TreeEntry *ea = (TreeEntry *)a;
    TreeEntry *eb = (TreeEntry *)b;
    return strcmp(ea->name, eb->name);
}

int tree_serialize(const Tree *tree, void **data_out, size_t *len_out) {
    Tree temp = *tree;

    qsort(temp.entries, temp.count, sizeof(TreeEntry), compare_entries);

    size_t total = 0;
    for (int i = 0; i < temp.count; i++) {
        total += snprintf(NULL, 0, "%o %s", temp.entries[i].mode, temp.entries[i].name) + 1 + HASH_SIZE;
    }

    char *buffer = malloc(total);
    if (!buffer) return -1;

    size_t offset = 0;

    for (int i = 0; i < temp.count; i++) {
        int written = sprintf(buffer + offset, "%o %s", temp.entries[i].mode, temp.entries[i].name);
        offset += written + 1;

        memcpy(buffer + offset, temp.entries[i].hash.hash, HASH_SIZE);
        offset += HASH_SIZE;
    }

    *data_out = buffer;
    *len_out = total;

    return 0;
}

int tree_parse(const void *data, size_t len, Tree *tree_out) {
    const char *ptr = data;
    size_t offset = 0;

    tree_out->count = 0;

    while (offset < len) {
        if (tree_out->count >= MAX_TREE_ENTRIES) return -1;

        TreeEntry *entry = &tree_out->entries[tree_out->count];

        unsigned int mode;
        char name[256];

        sscanf(ptr, "%o", &mode);
        entry->mode = mode;

        char *space = strchr(ptr, ' ');
        char *null = strchr(ptr, '\0');

        if (!space || !null) return -1;

        size_t name_len = null - space - 1;
        memcpy(name, space + 1, name_len);
        name[name_len] = '\0';

        strcpy(entry->name, name);

        size_t header_len = (null - ptr) + 1;
        ptr += header_len;
        offset += header_len;

        memcpy(entry->hash.hash, ptr, HASH_SIZE);
        ptr += HASH_SIZE;
        offset += HASH_SIZE;

        tree_out->count++;
    }

    return 0;
}

int tree_from_index(ObjectID *id_out) {
    Tree tree;
    tree.count = 0;

    void *data = NULL;
    size_t len = 0;

    if (tree_serialize(&tree, &data, &len) != 0) {
        return -1;
    }

    if (object_write(OBJ_TREE, data, len, id_out) != 0) {
        free(data);
        return -1;
    }

    free(data);
    return 0;
}
