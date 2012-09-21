# Copyright (c) 2012 Matthew Iselin, Rich Edelman
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

.global pc_acpi_wakeup, pc_acpi_save_state
.global pc_acpi_saveblock_addr, pc_acpi_gdt

.section .lowmem

.code16

# ACPI firmware wakeup code - should handle coming back from a sleep state, and
# restoring the system to functioning state. All 16-bit code, as we'll be in
# real mode.
pc_acpi_wakeup:
    # Ensure interrupts are entirely disabled, as we're going to jump into
    # protected mode shortly...
    cli

    # Clear up registers we will use.
    xor %eax, %eax
    xor %ecx, %ecx
    xor %edx, %edx
    xor %esi, %esi
    xor %edi, %edi

    # Load EDX with the real physical address of this code.
    movw %cs, %dx
    shl $4, %edx

    # Enable the A20 line, assuming it isn't already.
    call wait_a20
    mov $0xad, %al
    outb %al, $0x64

    call wait_a20
    mov $0xd0, %al
    outb %al, $0x64

    call wait_a20_2
    inb $0x60, %al
    push %eax

    call wait_a20
    mov $0xd1, %al
    outb %al, $0x64

    call wait_a20
    pop %eax
    or $2, %al
    out %al, $0x60

    call wait_a20
    mov $0xae, %al
    out %al, $0x64

    call wait_a20

    # Set up the GDT early.
    mov $pc_acpi_gdt, %edi
    andl $0xFFF, %edi
    add %edx, %edi

    # Set up the GDTR to point to the GDT.
    mov $pc_acpi_gdtr, %esi
    andl $0xFFF, %esi
    movw $23, %cs:(%esi)
    movl %edi, %cs:2(%esi)

    # Load the GDT now.
    lgdt %cs:(%esi)

    # Load the full 32-bit address of the save block.
    mov $pc_acpi_saveblock_addr, %esi
    andl $0xFFF, %esi
    movl %cs:(%esi), %esi

    # Convert into a segment:offset pair (DS:ESI)
    mov %esi, %ecx
    shr $4, %ecx
    movw %cx, %ds
    andl $0xFFF, %esi

    # Restore old CR2
    movl %ds:8(%esi), %eax
    movl %eax, %cr2

    # Restore old CR3 - page directory.
    movl %ds:12(%esi), %eax
    movl %eax, %cr3

    # Restore old CR4 - ensures large pages and such remain.
    movl %ds:16(%esi), %eax
    movl %eax, %cr4

    # Restore old code segment.
    movl %ds:20(%esi), %ebx

    # blah blah blah
    mov $pc_acpi_wakeup_32, %ecx
    andl $0xFFF, %ecx
    add %edx, %ecx

    # Restore old CR0
    movl %ds:(%esi), %eax
    movl %eax, %cr0

    # Because the address is in ECX, we can't use jmp %0x08, %ecx... retf it is!
    push %ebx
    push %ecx
    retfl

.code16

wait_a20:
    inb $0x64, %al
    test $2, %al
    jnz wait_a20
    ret

wait_a20_2:
    inb $0x64, %al
    test $1, %al
    jz wait_a20_2
    ret

.code32

# 32-bit step - this will be run in protected mode!
pc_acpi_wakeup_32:
    # Load up default segment registers to clear out the real mode cruft.
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    # Load the save block.
    mov $pc_acpi_saveblock_addr, %ecx
    movl (%ecx), %ecx

    # Load previous state segment registers now.
    movw 24(%ecx), %ax
    movw 28(%ecx), %ds
    movw 28(%ecx), %ss
    movw 32(%ecx), %es
    movw 36(%ecx), %fs
    movw 40(%ecx), %gs

    # Load general purpose registers.
    movl 44(%ecx), %ebx
    movl 48(%ecx), %esi
    movl 52(%ecx), %edi
    movl 56(%ecx), %ebp
    movl 60(%ecx), %esp

    # Load EFLAGS.
    movl 64(%ecx), %eax
    pushl %eax
    popf

    # Return address for the state save.
    mov 68(%ecx), %eax
    movl %eax, (%esp)
    movl $0, 4(%esp)

    # Done! CPU state restored - can return to the kernel proper now.
    movl $1, %eax
    ret

pc_acpi_save_state:
    movl 4(%esp), %eax

    # Critical system state first.
    mov %cr0, %ecx
    movl %ecx, (%eax)

    # Maybe one day, Intel will make CR1 a thing.
    mov $0, %ecx
    movl %ecx, 4(%eax)

    mov %cr2, %ecx
    movl %ecx, 8(%eax)

    mov %cr3, %ecx
    movl %ecx, 12(%eax)

    mov %cr4, %ecx
    movl %ecx, 16(%eax)

    # Data segments
    movw %cs, 20(%eax)
    movw %ds, 24(%eax)
    movw %ss, 28(%eax)
    movw %es, 32(%eax)
    movw %fs, 36(%eax)
    movw %gs, 40(%eax)

    # Registers not saved by caller.
    movl %ebx, 44(%eax)
    movl %esi, 48(%eax)
    movl %edi, 52(%eax)
    movl %ebp, 56(%eax)
    movl %esp, 60(%eax)

    # EFLAGS
    pushf
    movl (%esp), %ecx
    mov %ecx, 64(%eax)
    addl $4, %esp

    # Return address.
    movl (%esp), %ecx
    movl %ecx, 68(%eax)

    # Return 0 - 'state saved, go to sleep'. The wakeup function will return 1,
    # which will run the 'sleep finished, waking up' code path.
    movl $0, %eax
    ret

pc_acpi_saveblock_addr:
    .fill 4, 1, 0

.align 8
pc_acpi_gdtr:
    .fill 6, 1, 0

.align 4
pc_acpi_gdt:
    .fill 3, 8, 0
