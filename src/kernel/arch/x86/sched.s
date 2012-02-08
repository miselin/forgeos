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

.globl switch_context

switch_context:
	# Old context - don't save current if it's null.
	mov 4(%esp), %eax
	cmpl %eax, 0
	je .onlyload

	# EDI, ESI, EBX
	mov %edi, (%eax)
	mov %esi, 4(%eax)
	mov %ebx, 8(%eax)

	# EBP
	mov %ebp, 12(%eax)

	# ESP
	mov %esp, 16(%eax)

	# EIP
	mov (%esp), %ecx
	mov %ecx, 20(%eax)

.onlyload:

	mov 8(%esp), %eax
	mov (%eax), %edi
	mov 4(%eax), %esi
	mov 8(%eax), %ebx

	mov 12(%eax), %ebp
	mov 16(%eax), %esp
	addl $4, %esp

	mov 20(%eax), %ecx
	jmp *%ecx
