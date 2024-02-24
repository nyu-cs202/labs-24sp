###############################################################################
# BOOT ENTRY POINT
#
#   After the BIOS initializes the hardware on startup or system reset,
#   it loads the first 512-byte sector of the hard disk
#   into physical addresses 0x7C00-0x7DFF.
#   Then it jumps to address 0x7C00 -- and the OS starts running!
#
#   This file contains the code loaded at that address.
#   The `boot_start` routine switches the CPU out of compatibility mode,
#   which requires some subtle assembly programming.
#   Then it calls the `boot` routine to finish the booting
#   process.  This routine is defined in `boot.c`.
#
#   There is no need to understand this code in detail!
#   (You may stop reading now.)
#
###############################################################################


###############################################################################
# For Your Information: COMPATIBILITY MODES
#
#   The Intel x86 architecture has many compatibility modes, going back to
#   the 8086, which supported only 16-bit addresses.  When the BIOS calls
#   into the OS, it is running in the "most compatible" mode, 16-bit real
#   mode.  The machine acts like addresses are only 16 bits long,
#   there's no paging support, and there isn't even any support for
#   user-mode applications -- the processor is always privileged, so there's
#   no way to implement memory protection!  We want to get into "386" mode,
#   with 32-bit addresses and memory protection, as soon as possible.  And
#   that's what this code does.
#
#   Again, there is no need to understand this code in detail!
#
###############################################################################

# Define some constants
.set SEGSEL_BOOT_CODE,0x8       # code segment selector

.globl boot_start                               # Entry point
boot_start:
        .code16                         # This runs in real mode
        cli                             # Disable interrupts
        cld                             # String operations increment

        # All segments are initially 0.
        # Set up the stack pointer, growing downward from 0x7c00.
        movw    $boot_start, %sp

# Enable A20:
#   For fascinating historical reasons (related to the fact that
#   the earliest 8086-based PCs could only address 1MB of physical memory
#   and subsequent 80286-based PCs wanted to retain maximum compatibility),
#   physical address line 20 is tied to low when the machine boots.
#   Obviously this a bit of a drag for us, especially when trying to
#   address memory above 1MB.  This code undoes this.

seta20.1:       inb     $0x64, %al              # Get status
                testb   $0x2, %al               # Busy?
                jnz     seta20.1                # Yes
                movb    $0xd1, %al              # Command: Write
                outb    %al, $0x64              #  output port
seta20.2:       inb     $0x64, %al              # Get status
                testb   $0x2, %al               # Busy?
                jnz     seta20.2                # Yes
                movb    $0xdf, %al              # Command: Enable
                outb    %al, $0x60              #  A20

        .set INITIAL_PT,0x8000
        .set PTE_P,1
        .set PTE_W,2
        .set PTE_U,4
        .set PTE_PS,128

        .code16
init_pt:
        movl    $INITIAL_PT, %edi       # clear page table memory
        xorl    %eax, %eax
        movl    $(0x3000 / 4), %ecx
        rep stosl

        # set up initial page table.
        # 0x8000: L1 page table; entry 0 points to:
        # 0x9000: L2 page table; entry 0 points to:
        # 0xA000: L3 page table; entry 0 is a huge page covering 0-0x3FFFFFFF
        # Modern x86-64 processors support PTE_PS on L2 page entries,
        # but QEMU does not.
        movl    $INITIAL_PT, %edi       # set up page table: use a large page
        leal    0x1000 + PTE_P + PTE_W + PTE_U(%edi), %ecx
        movl    %ecx, (%edi)
        leal    0x2000 + PTE_P + PTE_W + PTE_U(%edi), %ecx
        movl    %ecx, 0x1000(%edi)
        movl    $(PTE_P + PTE_W + PTE_U + PTE_PS), -7(%ecx)
        movl    %edi, %cr3

# Switch from real to protected mode:
#   Up until now, there's been no protection, so we've gotten along perfectly
#   well without explicitly telling the processor how to translate addresses.
#   When we switch to protected mode, this is no longer true!
#   We need at least to set up some "segments" that tell the processor it's
#   OK to run code at any address, or write to any address.
#   The 'gdt' and 'gdtdesc' tables below define these segments.
#   This code loads them into the processor.
#   We need this setup to ensure the transition to protected mode is smooth.

        .set CR0_PE,0x1                 # protected mode enable
        .set CR0_WP,0x10000             # write-protect
        .set CR0_PG,0x80000000          # paging enable
        .set CR4_PSE,0x10
        .set CR4_PAE,0x20
        .set MSR_IA32_EFER,0xC0000080
        .set IA32_EFER_SCE,1            # enable syscall/sysret
        .set IA32_EFER_LME,0x100        # enable 64-bit mode
        .set IA32_EFER_NXE,0x800

real_to_prot:
        movl    %cr4, %eax              # enable physical address extensions
        orl     $(CR4_PSE | CR4_PAE), %eax
        movl    %eax, %cr4

        movl    $MSR_IA32_EFER, %ecx    # turn on 64-bit mode
        rdmsr
        orl     $(IA32_EFER_LME | IA32_EFER_SCE | IA32_EFER_NXE), %eax
        wrmsr

        movl    %cr0, %eax              # turn on protected mode
        orl     $(CR0_PE | CR0_WP | CR0_PG), %eax
        movl    %eax, %cr0

        lgdt    gdtdesc                 # load GDT

        # CPU magic: jump to relocation, flush prefetch queue, and
        # reload %cs.  Has the effect of just jmp to the next
        # instruction, but simultaneously loads CS with
        # $SEGSEL_BOOT_CODE.
        ljmp    $SEGSEL_BOOT_CODE, $boot


# Segment descriptors
        .code32
        .p2align 2                           # force 4 byte alignment
gdt:    .word 0, 0, 0, 0                     # null
        .word 0, 0; .byte 0, 0x98, 0x20, 0   # code seg
gdtdesc:
        .word 0x0f                           # sizeof(gdt) - 1
        .long gdt                            # address gdt
        .long 0
