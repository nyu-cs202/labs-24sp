#ifndef WEENSYOS_X86_H
#define WEENSYOS_X86_H
#include "lib.h"

// x86.h: C code to interface with x86 hardware and CPU.
//
//   Contents:
//   - Memory and interrupt constants.
//   - x86_registers: Used in process descriptors to store x86 registers.
//   - x86 functions: C function wrappers for useful x86 instructions.
//   - Hardware structures: C structures and constants for initializing
//     x86 hardware, including the interrupt descriptor table.

// Paged memory constants
#define PAGEOFFBITS     12                   // # bits in page offset
#define PAGESIZE        (1 << PAGEOFFBITS)   // Size of page in bytes
#define PAGEINDEXBITS   9                    // # bits in a page index level
#define NPAGETABLEENTRIES (1 << PAGEINDEXBITS) // # entries in page table page
#define PAGENUMBER(ptr) ((int) ((uintptr_t) (ptr) >> PAGEOFFBITS))
#define PAGEADDRESS(pn) ((uintptr_t) (pn) << PAGEOFFBITS)

// Page table entry type and page table type
typedef uint64_t x86_64_pageentry_t;
typedef struct __attribute__((aligned(PAGESIZE))) x86_64_pagetable {
    x86_64_pageentry_t entry[NPAGETABLEENTRIES];
} x86_64_pagetable;

// Parts of a paged address: page index, page offset
static inline int pageindex(uintptr_t addr, int level) {
    assert(level >= 0 && level <= 3);
    return (int) (addr >> (PAGEOFFBITS + (3 - level) * PAGEINDEXBITS)) & 0x1FF;
}
#define PAGEINDEX(addr, level) (pageindex((uintptr_t) (addr), (level)))
#define L1PAGEINDEX(addr)      (pageindex((uintptr_t) (addr), 0))
#define L2PAGEINDEX(addr)      (pageindex((uintptr_t) (addr), 1))
#define L3PAGEINDEX(addr)      (pageindex((uintptr_t) (addr), 2))
#define L4PAGEINDEX(addr)      (pageindex((uintptr_t) (addr), 3))
#define PAGEOFFMASK            ((uintptr_t) (PAGESIZE - 1))
#define PAGEOFFSET(addr)       ((uintptr_t) (addr) & PAGEOFFMASK)

// The physical address contained in a page table entry
#define PTE_ADDR(pageentry)     ((uintptr_t) (pageentry) & ~0xFFFUL)

// Page table entry flags
#define PTE_FLAGS(pageentry)    ((x86_64_pageentry_t) (pageentry) & 0xFFFU)
// - Permission flags: define whether page is accessible
#define PTE_P   ((x86_64_pageentry_t) 1)    // entry is Present
#define PTE_W   ((x86_64_pageentry_t) 2)    // entry is Writeable
#define PTE_U   ((x86_64_pageentry_t) 4)    // entry is User-accessible
// - Accessed flags: automatically turned on by processor
#define PTE_A   ((x86_64_pageentry_t) 32)   // entry was Accessed (read/written)
#define PTE_D   ((x86_64_pageentry_t) 64)   // entry was Dirtied (written)
#define PTE_PS  ((x86_64_pageentry_t) 128)  // entry has a large Page Size
// - There are other flags too!

// Page fault error flags
// These bits are stored in x86_registers::reg_err after a page fault trap.
#define PFERR_PRESENT   0x1             // Fault happened due to a protection
                                        //   violation (rather than due to a
                                        //   missing page)
#define PFERR_WRITE     0x2             // Fault happened on a write
#define PFERR_USER      0x4             // Fault happened in an application
                                        //   (user mode) (rather than kernel)


// Interrupt numbers
#define INT_DIVIDE      0x0         // Divide error
#define INT_DEBUG       0x1         // Debug exception
#define INT_BREAKPOINT  0x3         // Breakpoint
#define INT_OVERFLOW    0x4         // Overflow
#define INT_BOUNDS      0x5         // Bounds check
#define INT_INVALIDOP   0x6         // Invalid opcode
#define INT_DOUBLEFAULT 0x8         // Double fault
#define INT_INVALIDTSS  0xa         // Invalid TSS
#define INT_SEGMENT     0xb         // Segment not present
#define INT_STACK       0xc         // Stack exception
#define INT_GPF         0xd         // General protection fault
#define INT_PAGEFAULT   0xe         // Page fault


// struct x86_64_registers
//     A complete set of x86-64 general-purpose registers, plus some
//     special-purpose registers. The order and contents are defined to make
//     it more convenient to use important x86-64 instructions.

typedef struct x86_64_registers {
    uint64_t reg_rax;
    uint64_t reg_rcx;
    uint64_t reg_rdx;
    uint64_t reg_rbx;
    uint64_t reg_rbp;
    uint64_t reg_rsi;
    uint64_t reg_rdi;
    uint64_t reg_r8;
    uint64_t reg_r9;
    uint64_t reg_r10;
    uint64_t reg_r11;
    uint64_t reg_r12;
    uint64_t reg_r13;
    uint64_t reg_r14;
    uint64_t reg_r15;
    uint64_t reg_fs;
    uint64_t reg_gs;

    uint64_t reg_intno;         // (3) Interrupt number and error
    uint64_t reg_err;           // code (optional; supplied by x86
                                // interrupt mechanism)

    uint64_t reg_rip;		// (4) Task status: instruction pointer,
    uint16_t reg_cs;		// code segment, flags, stack
    uint16_t reg_padding2[3];	// in the order required by `iret`
    uint64_t reg_rflags;
    uint64_t reg_rsp;
    uint16_t reg_ss;
    uint16_t reg_padding3[3];
} x86_64_registers;


// x86 functions: Inline C functions that execute useful x86 instructions.
//
//      Also some macros corresponding to x86 register flag bits.

#define DECLARE_X86_FUNCTION(function_prototype) \
        static inline function_prototype __attribute__((always_inline))

DECLARE_X86_FUNCTION(void       breakpoint(void));
DECLARE_X86_FUNCTION(uint8_t    inb(int port));
DECLARE_X86_FUNCTION(void       insb(int port, void* addr, int cnt));
DECLARE_X86_FUNCTION(uint16_t   inw(int port));
DECLARE_X86_FUNCTION(void       insw(int port, void* addr, int cnt));
DECLARE_X86_FUNCTION(uint32_t   inl(int port));
DECLARE_X86_FUNCTION(void       insl(int port, void* addr, int cnt));
DECLARE_X86_FUNCTION(void       outb(int port, uint8_t data));
DECLARE_X86_FUNCTION(void       outsb(int port, const void* addr, int cnt));
DECLARE_X86_FUNCTION(void       outw(int port, uint16_t data));
DECLARE_X86_FUNCTION(void       outsw(int port, const void* addr, int cnt));
DECLARE_X86_FUNCTION(void       outsl(int port, const void* addr, int cnt));
DECLARE_X86_FUNCTION(void       outl(int port, uint32_t data));
DECLARE_X86_FUNCTION(void       invlpg(void* addr));
DECLARE_X86_FUNCTION(void       lidt(void* p));
DECLARE_X86_FUNCTION(void       lldt(uint16_t sel));
DECLARE_X86_FUNCTION(void       ltr(uint16_t sel));
DECLARE_X86_FUNCTION(void       lcr0(uint32_t val));
DECLARE_X86_FUNCTION(uint32_t   rcr0(void));
DECLARE_X86_FUNCTION(uintptr_t  rcr2(void));
DECLARE_X86_FUNCTION(void       lcr3(uintptr_t val));
DECLARE_X86_FUNCTION(uintptr_t  rcr3(void));
DECLARE_X86_FUNCTION(void       lcr4(uint64_t val));
DECLARE_X86_FUNCTION(uint64_t   rcr4(void));
DECLARE_X86_FUNCTION(void       cli(void));
DECLARE_X86_FUNCTION(void       sti(void));
DECLARE_X86_FUNCTION(void       tlbflush(void));
DECLARE_X86_FUNCTION(uint32_t   read_eflags(void));
DECLARE_X86_FUNCTION(void       write_eflags(uint32_t eflags));
DECLARE_X86_FUNCTION(uintptr_t  read_rbp(void));
DECLARE_X86_FUNCTION(uintptr_t  read_rsp(void));
DECLARE_X86_FUNCTION(void       cpuid(uint32_t info, uint32_t* eaxp,
                                      uint32_t* ebxp, uint32_t* ecxp,
                                      uint32_t* edxp));
DECLARE_X86_FUNCTION(uint64_t   read_cycle_counter(void));

// %cr0 flag bits (useful for lcr0() and rcr0())
#define CR0_PE                  0x00000001      // Protection Enable
#define CR0_MP                  0x00000002      // Monitor coProcessor
#define CR0_EM                  0x00000004      // Emulation
#define CR0_TS                  0x00000008      // Task Switched
#define CR0_ET                  0x00000010      // Extension Type
#define CR0_NE                  0x00000020      // Numeric Errror
#define CR0_WP                  0x00010000      // Write Protect
#define CR0_AM                  0x00040000      // Alignment Mask
#define CR0_NW                  0x20000000      // Not Writethrough
#define CR0_CD                  0x40000000      // Cache Disable
#define CR0_PG                  0x80000000      // Paging

// eflags bits (useful for read_eflags() and write_eflags())
#define EFLAGS_CF               0x00000001      // Carry Flag
#define EFLAGS_PF               0x00000004      // Parity Flag
#define EFLAGS_AF               0x00000010      // Auxiliary carry Flag
#define EFLAGS_ZF               0x00000040      // Zero Flag
#define EFLAGS_SF               0x00000080      // Sign Flag
#define EFLAGS_TF               0x00000100      // Trap Flag
#define EFLAGS_IF               0x00000200      // Interrupt Flag
#define EFLAGS_DF               0x00000400      // Direction Flag
#define EFLAGS_OF               0x00000800      // Overflow Flag
#define EFLAGS_IOPL_MASK        0x00003000      // I/O Privilege Level bitmask
#define EFLAGS_IOPL_0           0x00000000      //   IOPL == 0
#define EFLAGS_IOPL_1           0x00001000      //   IOPL == 1
#define EFLAGS_IOPL_2           0x00002000      //   IOPL == 2
#define EFLAGS_IOPL_3           0x00003000      //   IOPL == 3
#define EFLAGS_NT               0x00004000      // Nested Task
#define EFLAGS_RF               0x00010000      // Resume Flag
#define EFLAGS_VM               0x00020000      // Virtual 8086 mode
#define EFLAGS_AC               0x00040000      // Alignment Check
#define EFLAGS_VIF              0x00080000      // Virtual Interrupt Flag
#define EFLAGS_VIP              0x00100000      // Virtual Interrupt Pending
#define EFLAGS_ID               0x00200000      // ID flag

static inline void breakpoint(void) {
    asm volatile("int3");
}

static inline uint8_t inb(int port) {
    uint8_t data;
    asm volatile("inb %w1,%0" : "=a" (data) : "d" (port));
    return data;
}

static inline void insb(int port, void* addr, int cnt) {
    asm volatile("cld\n\trepne\n\tinsb"
                 : "=D" (addr), "=c" (cnt)
                 : "d" (port), "0" (addr), "1" (cnt)
                 : "memory", "cc");
}

static inline uint16_t inw(int port) {
    uint16_t data;
    asm volatile("inw %w1,%0" : "=a" (data) : "d" (port));
    return data;
}

static inline void insw(int port, void* addr, int cnt) {
    asm volatile("cld\n\trepne\n\tinsw"
                 : "=D" (addr), "=c" (cnt)
                 : "d" (port), "0" (addr), "1" (cnt)
                 : "memory", "cc");
}

static inline uint32_t inl(int port) {
    uint32_t data;
    asm volatile("inl %w1,%0" : "=a" (data) : "d" (port));
    return data;
}

static inline void insl(int port, void* addr, int cnt) {
    asm volatile("cld\n\trepne\n\tinsl"
                 : "=D" (addr), "=c" (cnt)
                 : "d" (port), "0" (addr), "1" (cnt)
                 : "memory", "cc");
}

static inline void outb(int port, uint8_t data) {
    asm volatile("outb %0,%w1" : : "a" (data), "d" (port));
}

static inline void outsb(int port, const void* addr, int cnt) {
    asm volatile("cld\n\trepne\n\toutsb"
                 : "=S" (addr), "=c" (cnt)
                 : "d" (port), "0" (addr), "1" (cnt)
                 : "cc");
}

static inline void outw(int port, uint16_t data) {
    asm volatile("outw %0,%w1" : : "a" (data), "d" (port));
}

static inline void outsw(int port, const void* addr, int cnt) {
    asm volatile("cld\n\trepne\n\toutsw"
                 : "=S" (addr), "=c" (cnt)
                 : "d" (port), "0" (addr), "1" (cnt)
                 : "cc");
}

static inline void outsl(int port, const void* addr, int cnt) {
    asm volatile("cld\n\trepne\n\toutsl"
                 : "=S" (addr), "=c" (cnt)
                 : "d" (port), "0" (addr), "1" (cnt)
                 : "cc");
}

static inline void outl(int port, uint32_t data) {
    asm volatile("outl %0,%w1" : : "a" (data), "d" (port));
}

static inline void invlpg(void* addr) {
    asm volatile("invlpg (%0)" : : "r" (addr) : "memory");
}

static inline void lidt(void* p) {
    asm volatile("lidt (%0)" : : "r" (p));
}

static inline void lldt(uint16_t sel) {
    asm volatile("lldt %0" : : "r" (sel));
}

static inline void ltr(uint16_t sel) {
    asm volatile("ltr %0" : : "r" (sel));
}

static inline void lcr0(uint32_t val) {
    uint64_t xval = val;
    asm volatile("movq %0,%%cr0" : : "r" (xval));
}

static inline uint32_t rcr0(void) {
    uint64_t val;
    asm volatile("movq %%cr0,%0" : "=r" (val));
    return val;
}

static inline uintptr_t rcr2(void) {
    uintptr_t val;
    asm volatile("movq %%cr2,%0" : "=r" (val));
    return val;
}

static inline void lcr3(uintptr_t val) {
    asm volatile("" : : : "memory");
    asm volatile("movq %0,%%cr3" : : "r" (val) : "memory");
}

static inline uintptr_t rcr3(void) {
    uintptr_t val;
    asm volatile("movq %%cr3,%0" : "=r" (val));
    return val;
}

static inline void lcr4(uint64_t val) {
    asm volatile("movq %0,%%cr4" : : "r" (val));
}

static inline uint64_t rcr4(void) {
    uint64_t cr4;
    asm volatile("movl %%cr4,%0" : "=r" (cr4));
    return cr4;
}

static inline void cli(void) {
    asm volatile("cli");
}

static inline void sti(void) {
    asm volatile("sti");
}

static inline void tlbflush(void) {
    uint32_t cr3;
    asm volatile("movl %%cr3,%0" : "=r" (cr3));
    asm volatile("movl %0,%%cr3" : : "r" (cr3));
}

static inline uint32_t read_eflags(void) {
    uint64_t eflags;
    asm volatile("pushfq; popq %0" : "=r" (eflags));
    return eflags;
}

static inline void write_eflags(uint32_t eflags) {
    uint64_t rflags = eflags; // really only lower 32 bits are used
    asm volatile("pushq %0; popfq" : : "r" (rflags));
}

static inline uintptr_t read_rbp(void) {
    uintptr_t rbp;
    asm volatile("movq %%rbp,%0" : "=r" (rbp));
    return rbp;
}

static inline uintptr_t read_rsp(void) {
    uintptr_t rsp;
    asm volatile("movq %%rsp,%0" : "=r" (rsp));
    return rsp;
}

static inline void cpuid(uint32_t info, uint32_t* eaxp, uint32_t* ebxp,
                         uint32_t* ecxp, uint32_t* edxp) {
    uint32_t eax, ebx, ecx, edx;
    asm volatile("cpuid"
                 : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx)
                 : "a" (info));
    if (eaxp) {
        *eaxp = eax;
    }
    if (ebxp) {
        *ebxp = ebx;
    }
    if (ecxp) {
        *ecxp = ecx;
    }
    if (edxp) {
        *edxp = edx;
    }
}

static inline uint64_t read_cycle_counter(void) {
    uint64_t tsc;
    asm volatile("rdtsc" : "=A" (tsc));
    return tsc;
}

static inline uint32_t fetch_and_addl(uint32_t* object, uint32_t addend) {
    asm volatile("lock; xaddl %0, %1"
                 : "+r" (addend), "+m" (*object)
                 : : "cc");
    return addend;
}

static inline void fence() {
    asm volatile("" : : : "memory");
}


// Hardware definitions: C structures and constants for initializing x86
// hardware, particularly gate descriptors (loaded into the interrupt
// descriptor table) and segment descriptors.

// Pseudo-descriptors used for LGDT, LLDT, and LIDT instructions
typedef struct __attribute__((packed, aligned(2))) x86_64_pseudodescriptor {
    uint16_t pseudod_limit;            // Limit
    uint64_t pseudod_base;             // Base address
} x86_64_pseudodescriptor;

// Task state structure defines kernel stack for interrupt handlers
typedef struct __attribute__((packed, aligned(8))) x86_64_taskstate {
    uint32_t ts_reserved0;
    uint64_t ts_rsp[3];
    uint64_t ts_ist[7];
    uint64_t ts_reserved1;
    uint16_t ts_reserved2;
    uint16_t ts_iomap_base;
} x86_64_taskstate;

// Gate descriptor structure defines interrupt handlers
typedef struct x86_64_gatedescriptor {
    uint64_t gd_low;
    uint64_t gd_high;
} x86_64_gatedescriptor;

// Segment bits
#define X86SEG_S        (1UL << 44)
#define X86SEG_P        (1UL << 47)
#define X86SEG_L        (1UL << 53)
#define X86SEG_DB       (1UL << 54)
#define X86SEG_G        (1UL << 55)

// Application segment type bits
#define X86SEG_A        (0x1UL << 40) // Accessed
#define X86SEG_R        (0x2UL << 40) // Readable (code segment)
#define X86SEG_W        (0x2UL << 40) // Writable (data segment)
#define X86SEG_C        (0x4UL << 40) // Conforming (code segment)
#define X86SEG_E        (0x4UL << 40) // Expand-down (data segment)
#define X86SEG_X        (0x8UL << 40) // Executable (== is code segment)

// System segment/interrupt descriptor types
#define X86SEG_TSS        (0x9UL << 40)
#define X86GATE_CALL      (0xCUL << 40)
#define X86GATE_INTERRUPT (0xEUL << 40)
#define X86GATE_TRAP      (0xFUL << 40)

// Keyboard programmed I/O
#define KEYBOARD_STATUSREG      0x64
#define KEYBOARD_STATUS_READY   0x01
#define KEYBOARD_DATAREG        0x60

#endif /* !WEENSYOS_X86_64_H */
