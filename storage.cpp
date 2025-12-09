#include "storage.h"
#include <Arduino.h>

#include "pico/stdlib.h"
#include <hardware/flash.h>
#include <hardware/sync.h>

#define FLASH_TARGET_OFFSET (512u * 1024u) // 512 KB
static_assert((FLASH_TARGET_OFFSET % FLASH_SECTOR_SIZE) == 0,
              "FLASH_TARGET_OFFSET must be sector-aligned");
storage_t storage = {0};
static uint8_t flash_buffer[FLASH_SECTOR_SIZE] __attribute__((aligned(4)));

bool storage_load() {
  const uint8_t *flash_ptr = (const uint8_t *)(XIP_BASE + FLASH_TARGET_OFFSET);

  memcpy(&storage, flash_ptr, sizeof(storage));

  if (storage.magic != MAGIC) {
    storage.magic = MAGIC;
    for (int i = 0; i < MAX_SENSORS; ++i) {
      storage.threshold[i] = DEFAULT_MOISTURE_THRESHOLD;
    }

    bool ok = storage_save();
    if (!ok) {
      MSerial.println("storage_load: failed to write default storage to flash");
      return false;
    } else {
      MSerial.println("storage_load: wrote default storage to flash");
    }
  } else {
    MSerial.println("storage_load: loaded storage from flash");
  }

  return true;
}

bool storage_save() {
#if !defined(ARDUINO_ARCH_RP2040)
  return false;
#else
  const size_t data_size = sizeof(storage);

  if (data_size > FLASH_SECTOR_SIZE) {
    MSerial.println("storage_save: storage struct is larger than a sector");
    return false;
  }

  memset(flash_buffer, 0xFF, FLASH_SECTOR_SIZE);
  memcpy(flash_buffer, &storage, data_size);

  uint32_t irq_state = save_and_disable_interrupts();

  flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);

  flash_range_program(FLASH_TARGET_OFFSET, flash_buffer, FLASH_SECTOR_SIZE);

  restore_interrupts(irq_state);

  const uint8_t *flash_ptr = (const uint8_t *)(XIP_BASE + FLASH_TARGET_OFFSET);
  if (memcmp(flash_ptr, flash_buffer, data_size) != 0) {
    MSerial.println("storage_save: verification failed");
    return false;
  }

  MSerial.println("storage_save: success");
  return true;
#endif
}
