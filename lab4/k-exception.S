###############################################################################
# Exception handlers
#
#   Assembly code defining kernel exception handlers
#   (for interrupts, traps, and faults).

.text

# The entry_from_boot routine sets the stack pointer to the top of the
# OS kernel stack, then jumps to the `kernel` routine.

.globl entry_from_boot
entry_from_boot:
        movq $0x80000, %rsp
        movq %rsp, %rbp
        pushq $0
        popfq
        // Check for multiboot command line; if found pass it along.
        cmpl $0x2BADB002, %eax
        jne 1f
        testl $4, (%rbx)
        je 1f
        movl 16(%rbx), %edi
        jmp 2f
1:      movq $0, %rdi
2:      jmp kernel


# Interrupt handlers
.align 2

        .globl gpf_int_handler
gpf_int_handler:
        pushq $13               // trap number
        jmp generic_exception_handler

        .globl pagefault_int_handler
pagefault_int_handler:
        pushq $14
        jmp generic_exception_handler

        .globl timer_int_handler
timer_int_handler:
        pushq $0                // error code
        pushq $32
        jmp generic_exception_handler

sys48_int_handler:
        pushq $0
        pushq $48
        jmp generic_exception_handler

sys49_int_handler:
        pushq $0
        pushq $49
        jmp generic_exception_handler

sys50_int_handler:
        pushq $0
        pushq $50
        jmp generic_exception_handler

sys51_int_handler:
        pushq $0
        pushq $51
        jmp generic_exception_handler

sys52_int_handler:
        pushq $0
        pushq $52
        jmp generic_exception_handler

sys53_int_handler:
        pushq $0
        pushq $53
        jmp generic_exception_handler

sys54_int_handler:
        pushq $0
        pushq $54
        jmp generic_exception_handler

sys55_int_handler:
        pushq $0
        pushq $55
        jmp generic_exception_handler

sys56_int_handler:
        pushq $0
        pushq $56
        jmp generic_exception_handler

sys57_int_handler:
        pushq $0
        pushq $57
        jmp generic_exception_handler

sys58_int_handler:
        pushq $0
        pushq $58
        jmp generic_exception_handler

sys59_int_handler:
        pushq $0
        pushq $59
        jmp generic_exception_handler

sys60_int_handler:
        pushq $0
        pushq $60
        jmp generic_exception_handler

sys61_int_handler:
        pushq $0
        pushq $61
        jmp generic_exception_handler

sys62_int_handler:
        pushq $0
        pushq $62
        jmp generic_exception_handler

sys63_int_handler:
        pushq $0
        pushq $63
        jmp generic_exception_handler

        .globl default_int_handler
default_int_handler:
        pushq $0
        jmp generic_exception_handler


generic_exception_handler:
        pushq %gs
        pushq %fs
        pushq %r15
        pushq %r14
        pushq %r13
        pushq %r12
        pushq %r11
        pushq %r10
        pushq %r9
        pushq %r8
        pushq %rdi
        pushq %rsi
        pushq %rbp
        pushq %rbx
        pushq %rdx
        pushq %rcx
        pushq %rax
        movq %rsp, %rdi
        call exception
        # `exception` should never return.


        .globl exception_return
exception_return:
        movq %rdi, %rsp
        popq %rax
        popq %rcx
        popq %rdx
        popq %rbx
        popq %rbp
        popq %rsi
        popq %rdi
        popq %r8
        popq %r9
        popq %r10
        popq %r11
        popq %r12
        popq %r13
        popq %r14
        popq %r15
        popq %fs
        popq %gs
        addq $16, %rsp
        iretq


        # An array of function pointers to the interrupt handlers.
        .globl sys_int_handlers
sys_int_handlers:
        .quad sys48_int_handler
        .quad sys49_int_handler
        .quad sys50_int_handler
        .quad sys51_int_handler
        .quad sys52_int_handler
        .quad sys53_int_handler
        .quad sys54_int_handler
        .quad sys55_int_handler
        .quad sys56_int_handler
        .quad sys57_int_handler
        .quad sys58_int_handler
        .quad sys59_int_handler
        .quad sys60_int_handler
        .quad sys61_int_handler
        .quad sys62_int_handler
        .quad sys63_int_handler
