#ifndef OBJECT_H
#define OBJECT_H

#include "pes.h"

#define HASH_HEX_SIZE 64

#include <stddef.h>

int object_write(ObjectType type, const void *data, size_t size, ObjectID *id);
int object_read(const char *hash, void **data, size_t *size);

#endif
