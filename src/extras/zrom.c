// this is very WIP.
// the gb.h api needs to be changed a bit to make this more clean
// this can be done by breaking [GB_loadrom] into smaller
// functions, as currently, loadrom does too much in 1 function
// and because of this, a lot of repeated code in here.

// secondly, the buffers are static, and not attached to the core
// struct, which means it wont work for multi gb instances.
// i am not sure how else to handle it currently (whilst avoiding malloc!)


#include "zrom.h"
#include "lz4.h"
#include "../gb.h"
#include "../internal.h"

#include <string.h>

enum {
  MAX_BANKS = 0x100,
  BANK_SIZE = 1024*16,
};

static struct ZromHeader zheader = {0};
static struct ZromBankEntry entries[MAX_BANKS] = {0};
static uint8_t mempool[2][BANK_SIZE] = {0};
static uint8_t mempool_slot[2] = {0};
static bool slots[MAX_BANKS] = {0};
static const uint8_t* rom_data = NULL;


static struct MBC_RomBankInfo mbc_common_get_rom_bank(const struct GB_Core* gb, uint8_t bank);
static struct MBC_RomBankInfo mbc0_get_rom_bank(struct GB_Core* gb, uint8_t bank);
static struct MBC_RomBankInfo mbc1_get_rom_bank(struct GB_Core* gb, uint8_t bank);
static struct MBC_RomBankInfo mbc2_get_rom_bank(struct GB_Core* gb, uint8_t bank);
static struct MBC_RomBankInfo mbc3_get_rom_bank(struct GB_Core* gb, uint8_t bank);
static struct MBC_RomBankInfo mbc5_get_rom_bank(struct GB_Core* gb, uint8_t bank);
static void uncompress_bank_to_pool(size_t bank);


static struct MBC_RomBankInfo mbc_common_get_rom_bank(const struct GB_Core* gb, uint8_t bank) {
  uncompress_bank_to_pool(bank * gb->cart.rom_bank);

  struct MBC_RomBankInfo info = {0};
  const uint8_t* ptr = ptr = mempool[bank];

  for (size_t i = 0; i < GB_ARR_SIZE(info.entries); ++i) {
    info.entries[i].ptr = ptr + (0x1000 * i);
    info.entries[i].mask = 0x0FFF;
  }

  return info;
}

static struct MBC_RomBankInfo mbc0_get_rom_bank(struct GB_Core* gb, uint8_t bank) {
  return mbc_common_get_rom_bank(gb, bank);
}

// NOTE: this is a stub, it wont work yet!!!!!!!!
static struct MBC_RomBankInfo mbc1_get_rom_bank(struct GB_Core* gb, uint8_t bank) {
  return mbc_common_get_rom_bank(gb, bank);
}

static struct MBC_RomBankInfo mbc2_get_rom_bank(struct GB_Core* gb, uint8_t bank) {
  return mbc_common_get_rom_bank(gb, bank);
}

static struct MBC_RomBankInfo mbc3_get_rom_bank(struct GB_Core* gb, uint8_t bank) {
  return mbc_common_get_rom_bank(gb, bank);
}

static struct MBC_RomBankInfo mbc5_get_rom_bank(struct GB_Core* gb, uint8_t bank) {
  return mbc_common_get_rom_bank(gb, bank);
}

static void uncompress_bank_to_pool(size_t bank) {
  // if already compressed
  if (slots[bank]) {
    return;
  }

  // bank 0 is fixed to pool zero
  const size_t pool = bank == 0 ? 0 : 1;

  const struct ZromBankEntry entry = entries[bank];

  if (entry.flags & ZromEntryFlag_COMPRESSED) {
    LZ4_decompress_safe((char*)rom_data + entry.offset, (char*)mempool[pool], entry.size, sizeof(mempool[pool]));
  }
  else if (entry.flags & ZromEntryFlag_UNCOMPRESSED) {
    memcpy((char*)mempool[pool], (char*)rom_data + entry.offset, entry.size);
  }

  slots[mempool_slot[pool]] = false;
  mempool_slot[pool] = bank;
  slots[mempool_slot[pool]] = true;
}

struct GB_MbcInterface {
  struct MBC_RomBankInfo (*get_rom_bank)(struct GB_Core *gb, uint8_t bank);
};

// todo: don't do this, gb api should set an enum for the mbc type
// being used, and then we should use a switch to set.
// this saves 2KiB of space.
static const struct GB_MbcInterface MBC_SETUP_DATA[0x100] = {
  // MBC0
  [0x00] = {mbc0_get_rom_bank},
  // MBC1
  [0x01] = {mbc1_get_rom_bank},
  [0x02] = {mbc1_get_rom_bank},
  [0x03] = {mbc1_get_rom_bank},
  // MBC2
  [0x05] = {mbc2_get_rom_bank},
  [0x06] = {mbc2_get_rom_bank},
  // MBC3
  [0x0F] = {mbc3_get_rom_bank},
  [0x10] = {mbc3_get_rom_bank},
  [0x11] = {mbc3_get_rom_bank},
  [0x13] = {mbc3_get_rom_bank},
  // MBC5
  [0x19] = {mbc5_get_rom_bank},
  [0x1A] = {mbc5_get_rom_bank},
  [0x1B] = {mbc5_get_rom_bank},
  [0x1C] = {mbc5_get_rom_bank},
  [0x1D] = {mbc5_get_rom_bank},
  [0x1E] = {mbc5_get_rom_bank},
};

bool GB_loadrom_compressed(struct GB_Core* gb, const void* data, size_t size) {
  (void)size;

  rom_data = (const uint8_t*)data;

  memset(slots, false, sizeof(slots));

  memcpy(&zheader, rom_data, sizeof(zheader));

  memcpy(entries, rom_data + sizeof(zheader), zheader.banks * sizeof(struct ZromBankEntry));

  // load bank 0,1 immediatly
  uncompress_bank_to_pool(0);
  uncompress_bank_to_pool(1);

  // setup rom data for GB
  gb->cart.rom = mempool[0];

  const struct GB_CartHeader* header = GB_get_rom_header_ptr(gb);
  // cart_header_print(header);

  #define ROM_SIZE_MULT 0x8000
  gb->cart.rom_size = ROM_SIZE_MULT << header->rom_size;

  enum {
    GBC_ONLY = 0xC0,
    GBC_AND_DMG = 0x80,
    // not much is known about these types
    // these are not checked atm, but soon will be...
    PGB_1 = 0x84,
    PGB_2 = 0x88,

    SGB_FLAG = 0x03,
    NEW_LICENSEE_USED = 0x33
  };

  const char gbc_flag = header->title[sizeof(header->title) - 1];

  if ((gbc_flag & GBC_ONLY) == GBC_ONLY) {
    // check if the user wants to force gbc mode
    if (gb->config.system_type_config == GB_SYSTEM_TYPE_CONFIG_DMG) {
      printf("[ERROR] only GBC system supported but config forces DMG system!\n");
      return false;
    }

    if (gb->config.system_type_config == GB_SYSTEM_TYPE_CONFIG_DMG) {
      gb->system_type = GB_SYSTEM_TYPE_SGB;
    } else {
      gb->system_type = GB_SYSTEM_TYPE_GBC;
    }
  }
  else if ((gbc_flag & GBC_AND_DMG) == GBC_AND_DMG) {
    // this can be either set to GBC or DMG mode, check if the
    // user has set a preffrence
    if (gb->config.system_type_config == GB_SYSTEM_TYPE_CONFIG_GBC) {
      gb->system_type = GB_SYSTEM_TYPE_GBC;
    } else if (gb->config.system_type_config == GB_SYSTEM_TYPE_CONFIG_SGB) {
      gb->system_type = GB_SYSTEM_TYPE_SGB;
    } else {
      gb->system_type = GB_SYSTEM_TYPE_GBC;
      // gb->system_type = GB_SYSTEM_TYPE_DMG;
    }
  }
  else {
    // check if the user wants to force GBC mode
    if (gb->config.system_type_config == GB_SYSTEM_TYPE_CONFIG_GBC) {
      printf("[ERROR] only DMG system supported but config forces GBC system!\n");
      return false;
    }

    // if the user wants SGB system, we can set it here as all gb
    // games work on the SGB.
    if (gb->config.system_type_config == GB_SYSTEM_TYPE_CONFIG_SGB) {
      gb->system_type = GB_SYSTEM_TYPE_SGB;
    } else {
      gb->system_type = GB_SYSTEM_TYPE_DMG;
    }
  }

  // need to change the above if-else tree to support this
  // for now, try and detect SGB
  if (header->sgb_flag == SGB_FLAG && header->old_licensee_code == NEW_LICENSEE_USED) {
    printf("[INFO] game supports SGB!\n");
    // gb->system_type = GB_SYSTEM_TYPE_SGB;
  }

  // try and setup the mbc, this also implicitly sets up
  // gbc mode
  if (!GB_setup_mbc(&gb->cart, header)) {
    printf("failed to setup mbc!\n");
    return false;
  }

  const struct GB_MbcInterface* mbc_iface = &MBC_SETUP_DATA[header->cart_type];
  gb->cart.get_rom_bank = mbc_iface->get_rom_bank;

  GB_reset(gb);
  GB_setup_mmap(gb);

  // todo:
  // // set the palette!
  // if (GB_is_system_gbc(gb) == false) {
  //   GB_setup_palette(gb, header);
  // }
  // else {
  //   // TODO: remove this!
  //   // this is for testing...
  //   GB_Palette_fill_from_custom(GB_CUSTOM_PALETTE_CREAM, &gb->palette);
  // }

  return true;  
}
