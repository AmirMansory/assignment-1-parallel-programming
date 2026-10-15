#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <cpuid.h>



void extract_cpu_details(void) {
    unsigned int eax, ebx, ecx, edx;

    unsigned int max_std_leaf = __get_cpuid_max(0, NULL);
    unsigned int max_ext_leaf = __get_cpuid_max(0x80000000, NULL);

    printf("============================================================\n");
    printf(" 1. PROCESSOR NAME, MODEL, AND GENERATION\n");
    printf("============================================================\n");

    if (max_ext_leaf >= 0x80000004) {
        char brand[49];
        unsigned int* b = (unsigned int*)brand;
        __cpuid(0x80000002, b[0], b[1], b[2], b[3]);
        __cpuid(0x80000003, b[4], b[5], b[6], b[7]);
        __cpuid(0x80000004, b[8], b[9], b[10], b[11]);
        brand[48] = '\0';
        printf("Full CPU Name  : %s\n", brand);
    }

    __cpuid(1, eax, ebx, ecx, edx);
    unsigned int stepping   = eax & 0xF;
    unsigned int base_model = (eax >> 4) & 0xF;
    unsigned int base_fam   = (eax >> 8) & 0xF;
    unsigned int ext_model  = (eax >> 16) & 0xF;
    unsigned int ext_fam    = (eax >> 20) & 0xFF;

    unsigned int display_family = (base_fam == 0xF) ? (base_fam + ext_fam) : base_fam;
    unsigned int display_model  = (base_fam == 0x6 || base_fam == 0xF)
                                  ? ((ext_model << 4) + base_model)
                                  : base_model;

    printf("Architecture   : x86_64 (Family: 0x%X, Model: 0x%X [%u], Stepping: %u)\n\n",
           display_family, display_model, display_model, stepping);

    
    printf("============================================================\n");
    printf(" 2 & 3. CORES AND HYPER-THREADING (SMT)\n");
    printf("============================================================\n");

    bool htt_bit = (edx & (1 << 28)) != 0;
    unsigned int max_apic_ids = (ebx >> 16) & 0xFF;

    unsigned int smt_MaskWidth = 0;
    unsigned int threads_per_core_max = 1;
    unsigned int total_logical_cpus = 0;

    if (max_std_leaf >= 0x0B) {
        int subleaf = 0;
        while (1) {
            unsigned int a, b_reg, c, d;
            __cpuid_count(0x0B, subleaf, a, b_reg, c, d);
            unsigned int level_type = (c >> 8) & 0xFF;
            if (level_type == 0) break; 

            unsigned int count = b_reg & 0xFFFF;
            if (level_type == 1) { 
                threads_per_core_max = count;
                smt_MaskWidth = a & 0x1F;
            } else if (level_type == 2) {
                total_logical_cpus = count;
            }
            subleaf++;
        }
    }

    printf("Hyper-Threading Flag (EDX Bit 28) : %s\n", htt_bit ? "Supported" : "Not Supported");
    printf("Max Threads per P-Core (Leaf 0xB) : %u (%s)\n",
           threads_per_core_max,
           (threads_per_core_max > 1) ? "Hyper-Threading ACTIVE" : "Hyper-Threading DISABLED");
    printf("Total Logical Processors (Threads): %u\n", total_logical_cpus);
    printf("Max Addressable APIC IDs (Leaf 1) : %u\n", max_apic_ids);
    printf("Note for Hybrid CPU (i9-14900HX)  : 24 Physical Cores (8 P-Cores + 16 E-Cores)\n\n");


    printf("============================================================\n");
    printf(" 4. PROCESSOR FREQUENCY (Leaf 0x16 vs Manufacturer Spec)\n");
    printf("============================================================\n");

    if (max_std_leaf >= 0x16) {
        unsigned int fa = 0, fb = 0, fc = 0, fd = 0;
        __cpuid_count(0x16, 0, fa, fb, fc, fd);
        unsigned int base_mhz = fa & 0xFFFF;
        unsigned int max_mhz  = fb & 0xFFFF;
        unsigned int bus_mhz  = fc & 0xFFFF;

        printf("CPUID Leaf 0x16 Base Frequency : %u MHz\n", base_mhz);
        printf("CPUID Leaf 0x16 Max Frequency  : %u MHz\n", max_mhz);
        printf("CPUID Leaf 0x16 Bus Frequency  : %u MHz\n", bus_mhz);
        if (base_mhz == 0 && max_mhz == 0) {
            printf("  -> [Note: Returns 0 MHz because Windows Hypervisor (VBS) or\n");
            printf("     hybrid P/E-core firmware masks legacy single-domain Leaf 0x16]\n");
        }
        printf("Official Intel Spec (i9-14900HX): Base 2200 MHz (P) / 1600 MHz (E) | Max Turbo 5800 MHz\n\n");
    }

 
    printf("============================================================\n");
    printf(" 5. SIMD INSTRUCTION SET SUPPORT\n");
    printf("============================================================\n");

    __cpuid(1, eax, ebx, ecx, edx);
    bool sse    = (edx & (1 << 25)) != 0;
    bool sse2   = (edx & (1 << 26)) != 0;
    bool sse3   = (ecx & (1 << 0))  != 0;
    bool ssse3  = (ecx & (1 << 9))  != 0;
    bool sse4_1 = (ecx & (1 << 19)) != 0;
    bool sse4_2 = (ecx & (1 << 20)) != 0;
    bool avx    = (ecx & (1 << 28)) != 0;

    unsigned int eax7 = 0, ebx7 = 0, ecx7 = 0, edx7 = 0;
    if (max_std_leaf >= 7) {
        __cpuid_count(7, 0, eax7, ebx7, ecx7, edx7);
    }
    bool avx2 = (ebx7 & (1 << 5)) != 0;

    printf("a. SSE    : %s\n", sse    ? "Yes" : "No");
    printf("b. SSE2   : %s\n", sse2   ? "Yes" : "No");
    printf("c. SSE3   : %s (SSSE3: %s)\n", sse3 ? "Yes" : "No", ssse3 ? "Yes" : "No");
    printf("d. SSE4   : SSE4.1 = %s, SSE4.2 = %s\n", sse4_1 ? "Yes" : "No", sse4_2 ? "Yes" : "No");
    printf("e. AVX    : %s\n", avx    ? "Yes" : "No");
    printf("f. AVX2   : %s\n\n", avx2 ? "Yes" : "No");


    printf("============================================================\n");
    printf(" 6A. CACHE HIERARCHY (CPUID Leaf 0x04)\n");
    printf("============================================================\n");

    if (max_std_leaf >= 0x04) {
        int subleaf = 0;
        while (1) {
            unsigned int ca, cb, cc, cd;
            __cpuid_count(0x04, subleaf, ca, cb, cc, cd);
            unsigned int cache_type = ca & 0x1F;
            if (cache_type == 0) break; // No more caches

            unsigned int cache_level = (ca >> 5) & 0x7;
            unsigned int line_size   = (cb & 0xFFF) + 1;
            unsigned int partitions  = ((cb >> 12) & 0x3FF) + 1;
            unsigned int ways        = ((cb >> 22) & 0x3FF) + 1;
            unsigned int sets        = cc + 1;

            unsigned long long size_bytes = (unsigned long long)ways * partitions * line_size * sets;
            const char* type_str = (cache_type == 1) ? "Data Cache       " :
                                   (cache_type == 2) ? "Instruction Cache" :
                                   (cache_type == 3) ? "Unified Cache    " : "Other";

            if (size_bytes >= 1024 * 1024) {
                printf("L%u %s : %5llu MB (%2u-way, %2uB line, %6u sets)\n",
                       cache_level, type_str, size_bytes / (1024 * 1024), ways, line_size, sets);
            } else {
                printf("L%u %s : %5llu KB (%2u-way, %2uB line, %6u sets)\n",
                       cache_level, type_str, size_bytes / 1024, ways, line_size, sets);
            }
            subleaf++;
        }
    }
    printf("\n");

    printf("============================================================\n");
    printf(" 6B. TLB INFORMATION (CPUID Leaf 0x18)\n");
    printf("============================================================\n");

    if (max_std_leaf >= 0x18) {
        unsigned int max_tlb_subleaf = 0, tb, tc, td;
        __cpuid_count(0x18, 0, max_tlb_subleaf, tb, tc, td);

        for (unsigned int sub = 0; sub <= max_tlb_subleaf; ++sub) {
            unsigned int ta, t_ebx, t_ecx, t_edx;
            __cpuid_count(0x18, sub, ta, t_ebx, t_ecx, t_edx);

            unsigned int tlb_type = t_edx & 0x1F;
            if (tlb_type == 0) continue; // Null entry

            unsigned int tlb_level = (t_edx >> 5) & 0x7;
            unsigned int ways      = (t_ebx >> 16) & 0xFFFF;
            unsigned int sets      = t_ecx;
            unsigned int entries   = ways * sets;

            bool page_4k  = (t_ebx & (1 << 0)) != 0;
            bool page_2m  = (t_ebx & (1 << 1)) != 0;
            bool page_4m  = (t_ebx & (1 << 2)) != 0;
            bool page_1g  = (t_ebx & (1 << 3)) != 0;

            const char* tlb_str = (tlb_type == 1) ? "Data TLB       " :
                                  (tlb_type == 2) ? "Instruction TLB" :
                                  (tlb_type == 3) ? "Unified TLB    " :
                                  (tlb_type == 4) ? "Load-Only TLB  " :
                                  (tlb_type == 5) ? "Store-Only TLB " : "Other TLB      ";

            printf("L%u %s : %4u entries (%2u-way, %3u sets) | Pages: %s%s%s%s\n",
                   tlb_level, tlb_str, entries, ways, sets,
                   page_4k ? "4KB " : "",
                   page_2m ? "2MB " : "",
                   page_4m ? "4MB " : "",
                   page_1g ? "1GB " : "");
        }
    }
    printf("\n");
}

typedef union {
    uint8_t  u8[16];
    int8_t   i8[16];
    uint16_t u16[8];
    int16_t  i16[8];
    uint32_t u32[4];
    int32_t  i32[4];
    uint64_t u64[2];
    int64_t  i64[2];
} Data128;

void print_128bit(const Data128* data, const char* type) {
    printf("%-4s : [ ", type);

    if (strcmp(type, "u8") == 0) {
        for (int i = 0; i < 16; ++i) printf("%" PRIu8 " ", data->u8[i]);
    } else if (strcmp(type, "i8") == 0) {
        for (int i = 0; i < 16; ++i) printf("%" PRId8 " ", data->i8[i]);
    } else if (strcmp(type, "u16") == 0) {
        for (int i = 0; i < 8; ++i)  printf("%" PRIu16 " ", data->u16[i]);
    } else if (strcmp(type, "i16") == 0) {
        for (int i = 0; i < 8; ++i)  printf("%" PRId16 " ", data->i16[i]);
    } else if (strcmp(type, "u32") == 0) {
        for (int i = 0; i < 4; ++i)  printf("%" PRIu32 " ", data->u32[i]);
    } else if (strcmp(type, "i32") == 0) {
        for (int i = 0; i < 4; ++i)  printf("%" PRId32 " ", data->i32[i]);
    } else if (strcmp(type, "u64") == 0) {
        for (int i = 0; i < 2; ++i)  printf("%" PRIu64 " ", data->u64[i]);
    } else if (strcmp(type, "i64") == 0) {
        for (int i = 0; i < 2; ++i)  printf("%" PRId64 " ", data->i64[i]);
    } else {
        printf("Unknown type");
    }

    printf("]\n");
}

int main(void) {
    extract_cpu_details();

    printf("============================================================\n");
    printf(" PART 1.2: 128-BIT UNION MEMORY INTERPRETATION\n");
    printf("============================================================\n");

    Data128 sample;
    for (int i = 0; i < 16; ++i) {
        sample.u8[i] = (uint8_t)(10 + i * 16);
    }

    print_128bit(&sample, "u8");
    print_128bit(&sample, "i8");
    print_128bit(&sample, "u16");
    print_128bit(&sample, "i16");
    print_128bit(&sample, "u32");
    print_128bit(&sample, "i32");
    print_128bit(&sample, "u64");
    print_128bit(&sample, "i64");

    return 0;
}