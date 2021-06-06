// this example shows how you can load savefiles using mmap.
// the benifit with mmap is that all writes are saved the the file
// meaning that you don't have to keep track of when saves happened etc

// we will also load the romfile using mmap, in case you wanted to see
// how thats done as well ^^

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <gb.h>


// mmap uses file descriptors
static int rom_fd = -1;
static int sram_fd = -1;

// the mapped data is here!
static uint8_t* rom_data = NULL;
static uint8_t* sram_data = NULL;
static size_t rom_size = 0;
static size_t sram_size = 0;


static void cleanup();

// protection flags, pretty straight forward to work out what they do!
#define ROM_PROT (PROT_READ)
#define SRAM_PROT (PROT_READ | PROT_WRITE)

// lots of flag options, but i general, only MAP_PRIVATE or MAP_SHARED
// matter at all.
// private means the data isn't wrote back to file, shared means it is.
#define ROM_FLAGS (MAP_PRIVATE)
#define SRAM_FLAGS (MAP_SHARED)
// if the game uses ram, but not battery powered,
// then we don't need to save the data to a file.
// for this `MAP_ANONYMOUS` allows use to pass -1 for the fd entry.
// so in affect, mmap is basically a malloc at that point (kinda).
#define SRAM_NO_BATTERY_FLAGS (MAP_PRIVATE | MAP_ANONYMOUS)


int main(int argc, char** argv)
{
    if (argc < 3)
    {
        printf("usage: ./exe rompath.gb saveout.sav\n");
        goto fail;
    }

    // static because struct is kinda large, so lets not hurt the stack!
    static struct GB_Core gameboy;

    // this just memsets the struct
    GB_init(&gameboy);

    // load rom!
    {
        rom_fd = open(argv[1], O_RDONLY);

        if (rom_fd == -1)
        {
            perror("failed to open rom");
            goto fail;
        }

        struct stat s = {0};

        if (fstat(rom_fd, &s) == -1)
        {
            perror("failed to stat rom");
            goto fail;
        }

        rom_size = s.st_size;

        rom_data = (uint8_t*)mmap(NULL, rom_size, ROM_PROT, ROM_FLAGS, rom_fd, 0);
    
        if (rom_data == MAP_FAILED)
        {
            perror("failed to mmap rom");
            goto fail;
        }
    }

    // now to setup sram
    {
        // first check if the game even uses sram, and if its battery powered!
        struct GB_RomInfo info = {0};

        if (!GB_get_rom_info(rom_data, rom_size, &info))
        {
            printf("failed to get rom data, likely it's invalid!\n");
            goto fail;
        }

        // save the ram size, we'll need to use this later for mmap
        sram_size = info.ram_size;
        printf("sram_size %zu\n", sram_size);

        if (info.mbc_flags & MBC_FLAGS_RAM)
        {
            int mmap_flags = SRAM_NO_BATTERY_FLAGS;

            if (info.mbc_flags & MBC_FLAGS_BATTERY)
            {
                mmap_flags = SRAM_FLAGS;

                sram_fd = open(argv[2], O_RDWR | O_CREAT, 0644);

                if (rom_fd == -1)
                {
                    perror("failed to open sram");
                    goto fail;
                }

                // we should check for the file of the file now,
                // as it could already exist!
                // if its a new file, the size will == 0,
                // thus mmap reads / writes out cause a bus error.
                // to fix this (hacky) is to write out the data to fill the file
                struct stat s = {0};

                if (fstat(sram_fd, &s) == -1)
                {
                    perror("failed to stat sram");
                    goto fail;
                }

                if (s.st_size < (long int)sram_size)
                {
                    char page[1024] = {0};
                    
                    for (size_t i = 0; i < sram_size; i += sizeof(page))
                    {
                        int size = sizeof(page) > sram_size-i ? sram_size-i : sizeof(page);
                        write(sram_fd, page, size);
                    }
                }
            }
        
            sram_data = (uint8_t*)mmap(NULL, sram_size, SRAM_PROT, mmap_flags, sram_fd, 0);
    
            if (sram_data == MAP_FAILED)
            {
                perror("failed to mmap sram");
                goto fail;
            }

            GB_set_sram(&gameboy, sram_data, sram_size);
        }
    }

    // finally we can load the rom, hopefully all goes well!
    if (!GB_loadrom(&gameboy, rom_data, rom_size))
    {
        printf("somehow failed to loadrom, though this shouldve failed earlier...\n");
        goto fail;
    }

    // run for 30s worth. should be enough to populate sram
    for (int i = 0; i < 60*30; ++i)
    {
        GB_run_frame(&gameboy);
    }

    cleanup();

    return 0;

fail:
    cleanup();
    return -1;
}

void cleanup()
{
    if (rom_data)       { munmap(rom_data, rom_size); }
    if (sram_data)      { munmap(sram_data, sram_size); }

    if (rom_fd != -1)   { close(rom_fd); }
    if (sram_fd != -1)  { close(sram_fd); }
}
