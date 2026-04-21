#ifndef INDEX_H
#define INDEX_H

typedef struct {
    int dummy;
} Index;

int index_load(Index *index);
int index_add(Index *index, const char *filename);
int index_status(Index *index);

#endif
