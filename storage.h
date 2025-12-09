#ifndef STORAGE_H
#define STORAGE_H

#include "config.h"
#include <cstdint>

#define MAGIC 0xABCABCAB

struct storage_t {
  uint32_t magic;
  uint16_t threshold[MAX_SENSORS];
};

extern struct storage_t storage;

bool storage_load(void);
bool storage_save(void);

#endif
