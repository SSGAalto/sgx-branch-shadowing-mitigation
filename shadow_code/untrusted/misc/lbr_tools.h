/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */

#ifndef LBR_TOOLS_H
#define LBR_TOOLS_H

/* NOTE! This is Skylake specific! */
#define lbr_count 32
/* These are in arch/x86/include/asm/msr-index.h */
#define MSR_LBR_NHM_FROM 0x00000680
#define MSR_LBR_NHM_TO 0x000006c0
#define MSR_LBR_INFO_0 0x00000dc0
#define MSR_LBR_SELECT 0x000001c8
#define MSR_LBR_TOS 0x000001c9
#define MSR_IA32_DEBUGCTLMSR 0x000001d9

#define addr_64mask ((1UL << 48) - 1)
#define mispred_32mask (1UL << 31)
#define cycles_32mask ((1UL << 16) - 1)

struct __attribute((__packed__)) lbr_data {
    unsigned int msrf;
    unsigned int msrt;
    unsigned int axf;
    unsigned int dxf;
    unsigned int axt;
    unsigned int dxt;
    unsigned int MSR_LBR_INFO_a;
    unsigned int MSR_LBR_INFO_d;
};

#ifdef __cplusplus
extern "C" {
#endif

int enable_LBR(void *d);
int disable_LBR(void *d);
int dump_LBR(struct lbr_data *);
void all_disable_LBR(void);
void all_enable_LBR(void);

__always_inline
static int read_lbr_tos(void);

__always_inline
static void read_lbr_entry(struct lbr_data *data, int f, int t, int i);

__always_inline
static unsigned long lbr_data_get_from(struct lbr_data *data);

__always_inline
static unsigned long lbr_data_get_to(struct lbr_data *data);

__always_inline
static unsigned int lbr_data_get_mispred(struct lbr_data *data);

__always_inline
static unsigned int lbr_data_get_cycle_count(struct lbr_data *data);

__always_inline
static void lbr_enable_inline(void);

__always_inline
static void lbr_disable_inline(void);

__always_inline
static void lbr_filter_inline(int edx, int eax);

__always_inline
static void read_lbr(struct lbr_data *data);

__always_inline
static int read_lbr_tos(void)
{
    int eax, edx;

    asm volatile (""
            "mov %[msr], %%ecx;"
            "rdmsr;"
            "mov %%eax, %[eax];"
            "mov %%edx, %[edx];"
            :
            [eax] "=g" (eax),
            [edx] "=g" (edx)
            :
            [msr] "g" (MSR_LBR_TOS)
            : "eax", "ecx", "edx"
            );

    return eax;
}


__always_inline
static void read_lbr_entry(struct lbr_data *data, int f, int t, int i)
{
    asm volatile    (
            "mov %[Msrf], %%ecx;"
            "rdmsr;"
            "mov %%eax, %[Axf];"
            "mov %%edx, %[Dxf];"
            "mov %[Msrt], %%ecx;"
            "rdmsr;"
            "mov %%eax, %[Axt];"
            "mov %%edx, %[Dxt];"
            :
            [Axf] "=g" (data->axf),
            [Dxf] "=g" (data->dxf),
            [Axt] "=g" (data->axt),
            [Dxt] "=g" (data->dxt)
            :
            [Msrf] "g" (f),
            [Msrt] "g" (t)
            : "%eax", "%ecx", "%edx"
            );

    asm volatile (""
            "mov %[msr], %%ecx;"
            "rdmsr;"
            "mov %%eax, %[eax];"
            "mov %%edx, %[edx];"
            :
            [eax] "=g" (data->MSR_LBR_INFO_a),
            [edx] "=g" (data->MSR_LBR_INFO_d)
            :
            [msr] "g" (MSR_LBR_INFO_0 + i)
            : "%eax", "%ecx", "%edx"
            );

    data->msrf = f;
    data->msrt = t;
}

__always_inline
static unsigned long lbr_data_get_from(struct lbr_data *data)
{
     return addr_64mask & ((((unsigned long) data->dxf) << 32UL) + data->axf);
}

__always_inline
static unsigned long lbr_data_get_to(struct lbr_data *data)
{
    return addr_64mask & ((((unsigned long) data->dxt) << 32UL) + data->axt);
}

__always_inline
static unsigned int lbr_data_get_mispred(struct lbr_data *data)
{
     return mispred_32mask & data->MSR_LBR_INFO_d;
}

__always_inline
static unsigned int lbr_data_get_cycle_count(struct lbr_data *data)
{
     return cycles_32mask & data->MSR_LBR_INFO_a;
}

__always_inline
static void lbr_enable_inline(void)
{
    asm volatile (
            "xor %%edx, %%edx;"
            "xor %%eax, %%eax;"
            "inc %%eax;"
            "mov %[msr], %%ecx;"
            "wrmsr;"
            : : [msr] "g" (MSR_IA32_DEBUGCTLMSR)
            : "%edx", "%eax", "%ecx");
}

__always_inline
static void lbr_disable_inline(void)
{
    asm volatile (
            "xor %%edx, %%edx;"
            "xor %%eax, %%eax;"
            "mov %[msr], %%ecx;"
            "wrmsr;"
            : : [msr] "g" (MSR_IA32_DEBUGCTLMSR)
            : "%edx", "%eax", "%ecx");
}

__always_inline
static void lbr_filter_inline(int edx, int eax)
{
    asm volatile (
            "mov %[Edx], %%edx;"
            "mov %[Eax], %%eax;"
            "mov %[msr], %%ecx;"
            "wrmsr;"
            : : [Edx] "g" (edx), [Eax] "g" (eax), [msr] "g" (MSR_LBR_SELECT)
            : "%edx", "%eax", "%ecx");
}

__always_inline
static void read_lbr(struct lbr_data *data)
{
	int i = read_lbr_tos();
    int o = lbr_count-1;
    const int msr_f = MSR_LBR_NHM_FROM;
    const int msr_t = MSR_LBR_NHM_TO;

#define read_next() {                                   \
    read_lbr_entry(&(data[o]), msr_f+i, msr_t+i, i);    \
    i = (i+1) % lbr_count;                              \
    o = (o+1) % lbr_count;                              \
}

    /* Don't loop, we don't want no branches! */
    read_next();
    read_next();
    read_next();
    read_next();
    read_next();
    read_next();
    read_next();
    read_next();
    read_next();
    read_next();
    read_next();
    read_next();
    read_next();
    read_next();
    read_next();
    read_next();
    read_next();
    read_next();
    read_next();
    read_next();
    read_next();
    read_next();
    read_next();
    read_next();
    read_next();
    read_next();
    read_next();
    read_next();
    read_next();
    read_next();
    read_next();
    read_next();

#undef read_next
}

#ifdef __cplusplus
}
#endif

#endif /* !LBR_TOOLS_H */
