#ifndef _ZROM_H_
#define _ZROM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "../types.h"


enum ZromEntryFlag {
  ZromEntryFlag_COMPRESSED = 1 << 0,
  // some banks don't compress at all, and end
  // up larger than 16KiB!.
  // for those, we keep the bank uncompressed
  // which allows for memcpy speed!
  ZromEntryFlag_UNCOMPRESSED = 1 << 1,
};

struct ZromHeader {
  uint32_t magic;
  uint16_t banks;
  uint16_t reserved;
};

struct ZromBankEntry {
  uint32_t offset;
  uint16_t size;
  uint8_t flags;
  uint8_t reserved;
};

GBAPI bool GB_loadrom_compressed(struct GB_Core* gb, const void* data, size_t size);

#ifdef __cplusplus
}
#endif

#endif // _ZROM_H_
