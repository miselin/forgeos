# Copyright (c) 2011 Matthew Iselin, Rich Edelman
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

.globl save_thread_context
.globl restore_thread_context

.extern arch_interrupts_get


save_thread_context:
	# Grab current context
	mov 4(%esp), %edx

	mov %edi, (%edx)
	mov %esi, 4(%edx)
	mov %ebx, 8(%edx)
	mov %ebp, 12(%edx)
	mov %esp, 16(%edx)
	mov (%esp), %ecx
	mov %ecx, 20(%edx)  # eip

	pushf
	pop %ecx
	mov %ecx, 24(%edx)  # eflags

	# restoring this context will return zero to indicate context switch return
	mov $1, %eax
	ret

restore_thread_context:
	# lock
	mov 8(%esp), %ecx

	# load context
	mov 4(%esp), %eax

	mov (%eax), %edi
	mov 4(%eax), %esi
	mov 8(%eax), %ebx
	mov 12(%eax), %ebp
	mov 16(%eax), %esp
	addl $4, %esp

	test %ecx, %ecx
	je .nolock

	lock decl (%ecx)

.nolock:

	# EFLAGS.
	mov 24(%eax), %ecx
	push %ecx
	popfl

	# Return 1 - we're restoring context
	mov 20(%eax), %ecx
	mov $0, %eax
	jmp *%ecx
