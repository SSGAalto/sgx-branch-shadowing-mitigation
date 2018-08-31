/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */


#ifndef noinline
#define noinline __attribute__((noinline))
#endif

// #define DO_LOTSA_NOPS_PADDING
#ifdef DO_LOTSA_NOPS_PADDING
#define nops_padding "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t" \
    "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
#else
#define nops_padding "nop\n\tnop\n\tnop\n\tnop\n\t"
#endif

#ifndef VICTIM_FUNC
#define VICTIM_FUNC(x) default_##x
#endif

/* The byte length of the function, taken from objdump */
#define victim_jne_len 142
#define victim_ret_len 0x138

#if defined(__cplusplus)
extern "C" {
#endif

noinline int VICTIM_FUNC(victim_jne)(int number)
{
    int retval;

    __asm__ volatile(""
    nops_padding
    "mov %[Number], %%eax\n\t"
    "and $0x1,%%eax\n\t"
    "test %%eax,%%eax\n\t"
    "jne 0f\n\t"
    nops_padding
    "movl $0x1, %[Retval]\n\t"
    "jmp 1f\n\t"
    nops_padding
    "0: movl $0x2, %[Retval]\n\t"
    "1:"
    :
    [Retval] "=m" (retval)
    :
    [Number] "g" (number)
    : );

    return retval;
}

void *VICTIM_FUNC(get_victim_jne_src)(void)
{
    int i;
    char *ptr = (char *)&VICTIM_FUNC(victim_jne);

    for (i = 0; i < victim_jne_len; i++) {
        if (ptr[i]   == '\x75')
            return &ptr[i];
    }
    return 0;
}

void *VICTIM_FUNC(get_victim_jne_dst)(void)
{
    int i;
    char *ptr = (char *)&VICTIM_FUNC(victim_jne);

    for (i = 0; i < victim_jne_len; i++) {
        if (ptr[i]   == '\xc7' &&
            ptr[i+1] == '\x45' &&
            //ptr[i+2] == '\xf4' &&
            ptr[i+3] == '\x02' &&
            ptr[i+4] == '\x00'
                )
            return &ptr[i];
    }
    return 0;
}

noinline int VICTIM_FUNC(victim_ret)(int number) {
    const void *const addr_retpoline = &&retpoline;
    const void *const addr_if = &&target_if;
    const void *const addr_else = &&target_else;
    const void *const addr_exit = &&exit_jump;

    asm volatile(""
                 "mov %[Number], %%eax\n\t"
                 "mov %[If], %%r14\n\t"      /* r14 <- target_if */
                 "and $0x1,%%eax\n\t"
                 "test %%eax, %%eax\n\t"
                 "cmova %[Else], %%r14\n\t"  /* r14 <- target_else */
                 "call *%[Retpoline]\n\t"
    nops_padding
    : :
    [Number] "m"(number),
    [Retpoline] "m"(addr_retpoline),
    [If] "r"(addr_if),
    [Else] "r"(addr_else)
    : "%eax", "r14" );

    target_if:
    asm volatile(""
                 "movl $0x1, %%eax\n\t"
    : : : );

    asm volatile(""
    nops_padding
    "jmp *%[Exit]\n\t"
    nops_padding
    : : [Exit] "r"(addr_exit) : "%eax" );

    target_else:
    asm volatile(""
                 "movl $0x2, %%eax\n\t"
    : : : );

    asm volatile(""
    nops_padding
    "jmp *%[Exit]\n\t"
    nops_padding
    : : [Exit] "r"(addr_exit) : "%eax" );

    retpoline:
    __asm__ volatile(""
    nops_padding
    "mov %%r14, (%%rsp)\n\t"
    "ret\n\t"
    nops_padding
    : : : );

    exit_jump:
    __asm__ volatile(""
                     "mov %%eax, %[Number]\n\t"
    : [Number] "=g"(number) : : );

    return number;
}

void *VICTIM_FUNC(get_victim_ret_retpoline)(void)
{
    int i;
    char *ptr = (char *)&VICTIM_FUNC(victim_ret);

    for (i = 0; i < victim_ret_len; i++) {
        if (ptr[i]   == '\xc3' && ptr[i+1] == '\x90')
            return &ptr[i];
    }
    return 0;
}

void *VICTIM_FUNC(get_victim_ret_else)(void)
{
    int i;
    char *ptr = (char *)&VICTIM_FUNC(victim_ret);

    for (i = 0; i < victim_ret_len; i++) {
        if (ptr[i] == '\x90' && ptr[i+1]   == '\xb8' && ptr[i+2] == '\x02')
            return &ptr[i+1];
    }
    return 0;
}

void *VICTIM_FUNC(get_victim_ret_if)(void)
{
    int i;
    char *ptr = (char *)&VICTIM_FUNC(victim_ret);

    for (i = 0; i < victim_ret_len; i++) {
        if (ptr[i] == '\x90' && ptr[i+1]   == '\xb8' && ptr[i+2] == '\x01')
            return &ptr[i+1];
    }

    return 0;
}

#if defined(__cplusplus)
}
#endif
