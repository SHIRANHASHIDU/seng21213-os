#ifndef PMM_H
#define PMM_H
#include <stdint.h>

#define PMM_FRAME_SIZE 4096
#define PMM_MAX_FRAMES 16384
#define PMM_MAX_E820_ENTRIES 32

typedef struct __attribute__((packed)) {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t acpi_ext;
} e820_entry_t;

void     pmm_init(void);
uint32_t pmm_alloc_frame(void);
void     pmm_free_frame(uint32_t phys_addr);
uint32_t pmm_total_frames(void);
uint32_t pmm_used_frames(void);
uint32_t pmm_free_frames(void);
int      pmm_selftest(void);

#endif
