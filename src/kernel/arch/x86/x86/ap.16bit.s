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

.global pc_ap_entry
.global pc_ap_pdir
.extern ap_startup

.section .lowmem

.code16

.align 4096

pc_ap_entry:
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

    # Try for the BIOS A20 enable.
    movw $0x2401, %ax
    int $0x15

    # Success?
    jnc .a20good

    # Enable the A20 line via the KBC
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

.a20good:

    # Set up the GDT early.
    mov $pc_ap_gdt, %edi
    andl $0xFFF, %edi
    add %edx, %edi

    # Set up the GDTR to point to the GDT.
    mov $pc_ap_gdtr, %esi
    andl $0xFFF, %esi
    movw $0x17, %cs:(%esi)
    movl %edi, %cs:2(%esi)

    # Load the GDT
    lgdt %cs:(%esi)

    # Load our page directory to use.
    mov $pc_ap_pdir, %esi
    andl $0xFFF, %esi
    movl %cs:(%esi), %esi
    mov %esi, %cr3

    # Load the 32-bit entry point.
    mov $pc_ap_entry_32, %ecx
    andl $0xFFF, %ecx
    add %edx, %ecx

    # Enable protected mode.
    mov %cr0, %eax
    or $0x1, %eax
    mov %eax, %cr0

    # Because the address is in ECX, we can't use jmp %0x08, %ecx... retf it is!
    pushl $0x08
    pushl %ecx
    retfl

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
pc_ap_entry_32:
    # Load up default segment registers to clear out the real mode cruft.
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    # TODO: proper stack.
    mov $end_stack - 4, %ebp
    mov $end_stack - 4, %esp

    # Enable paging before we jump to the kernel.
    mov %cr0, %eax
    or $0x80000000, %eax
    mov %eax, %cr0

    # Jump to the kernel entry point now.
    mov $ap_startup, %eax
    call *%eax

    cli
    h:
    hlt
    jmp h

# Initial GDT for the CPU
.align 4
pc_ap_gdt:
    .long 0x0
    .long 0x0

    # Code.
    .word 0xFFFF
    .word 0x0
    .byte 0x00
    .byte 0x98
    .byte 0xCF
    .byte 0x00

    # Data.
    .word 0xFFFF
    .word 0x0
    .byte 0x00
    .byte 0x92
    .byte 0xCF
    .byte 0x00
gdt_end:

.align 4
pc_ap_gdtr:
    .fill 6, 1, 0

pc_ap_pdir:
    .fill 4, 1, 0

stack:
    .fill 1024, 1, 0
end_stack:
