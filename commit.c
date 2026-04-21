#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "pes.h"
#include "object.h"
#include "commit.h"
#include "tree.h"

int object_write(ObjectType type, const void *data, size_t size, ObjectID *id_out);
void hash_to_hex(const ObjectID *id, char *hex_out);

int commit_create(const char *message, ObjectID *commit_id_out) {
    FILE *f = fopen(INDEX_FILE, "r");
    if (!f) return -1;

    char index_data[8192];
    size_t len = fread(index_data, 1, sizeof(index_data) - 1, f);
    fclose(f);
    index_data[len] = '\0';

    char commit_data[16384];

    const char *author = pes_author();
    uint64_t timestamp = time(NULL);

    ObjectID tree_id;
    if (tree_from_index(&tree_id) != 0) {
        return -1;
    }

    char tree_hex[HASH_HEX_SIZE + 1];
    hash_to_hex(&tree_id, tree_hex);

    FILE *hf_read = fopen(HEAD_FILE, "r");

    char parent_hex[HASH_HEX_SIZE + 1];
    int has_parent = 0;

    if (hf_read && fgets(parent_hex, sizeof(parent_hex), hf_read)) {
        parent_hex[strcspn(parent_hex, "\n")] = 0;
        has_parent = 1;
    }
    if (hf_read) fclose(hf_read);

    char parent_line[128] = "";

    if (has_parent) {
        snprintf(parent_line, sizeof(parent_line), "parent %s\n", parent_hex);
    }

    snprintf(commit_data, sizeof(commit_data),
        "tree %s\n"
        "%s"
        "author %s\n"
        "date %lu\n"
        "\n"
        "%s\n",
        tree_hex,
        parent_line,
        author,
        timestamp,
        message
    );

    size_t total_size = strlen(commit_data);

    if (object_write(OBJ_COMMIT, commit_data, total_size, commit_id_out) != 0) {
        return -1;
    }

    FILE *hf = fopen(HEAD_FILE, "w");
    if (!hf) return -1;

    char hex[HASH_HEX_SIZE + 1];
    hash_to_hex(commit_id_out, hex);

    fprintf(hf, "%s\n", hex);
    fclose(hf);

    return 0;
}

int commit_parse(const void *data, size_t len, Commit *commit_out) {
    const char *ptr = (const char *)data;

    memset(commit_out, 0, sizeof(Commit));

    char tree_hex[65];
    sscanf(ptr, "tree %64s\n", tree_hex);
    hex_to_hash(tree_hex, &commit_out->tree);

    const char *parent_line = strstr(ptr, "parent ");
    if (parent_line) {
        
        char parent_hex[HASH_HEX_SIZE + 1];

        if (sscanf(parent_line, "parent %64s", parent_hex) == 1) {
             hex_to_hash(parent_hex, &commit_out->parent);
             commit_out->has_parent = 1;
        }

        commit_out->has_parent = 1;
    } else {
        memset(&commit_out->parent, 0, sizeof(ObjectID));
        commit_out->has_parent = 0;
    }

    const char *author_line = strstr(ptr, "author ");
    if (author_line) {
        sscanf(author_line, "author %255[^\n]\n", commit_out->author);
    }

    const char *date_line = strstr(ptr, "date ");
    if (date_line) {
        sscanf(date_line, "date %lu\n", &commit_out->timestamp);
    }

    const char *msg = strstr(ptr, "\n\n");

    if (msg) {
        msg += 2;

        if (strncmp(msg, "author ", 7) == 0 || strncmp(msg, "date ", 5) == 0) {
            msg = strstr(msg, "\n\n");
            if (msg) msg += 2;
        }
    }

    if (msg) {
        strncpy(commit_out->message, msg, sizeof(commit_out->message) - 1);
        commit_out->message[sizeof(commit_out->message) - 1] = '\0';
    }

    commit_out->message[strcspn(commit_out->message, "\n")] = '\0';

    return 0;
}

int commit_walk(commit_walk_fn callback, void *ctx) {
    FILE *f = fopen(HEAD_FILE, "r");
    if (!f) return -1;

    char hex[HASH_HEX_SIZE + 1];
    if (!fgets(hex, sizeof(hex), f)) {
        fclose(f);
        return -1;
    }
    fclose(f);

    hex[strcspn(hex, "\n")] = 0;

    ObjectID current;
    if (hex_to_hash(hex, &current) != 0) return -1;

    while (1) {
        void *data;
        size_t size;

        char hex[HASH_HEX_SIZE + 1];
        hash_to_hex(&current, hex);

        if (object_read(hex, &data, &size) != 0) break;

        Commit c;
        if (commit_parse(data, size, &c) != 0) {
            free(data);
            break;
        }

        free(data);

        callback(&current, &c, ctx);

        if (!c.has_parent) break;

        current = c.parent;
    }

    return 0;
}
