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
    # Restore context if the 'save to' context is null.
    cmp r0, #0
    beq .restoreonly

    # Save current state.
    mov r12, lr
    stmia r0, {r4 - r11, r12, sp}^

.restoreonly:

    # Load new state.
    ldmia r1, {r4 - r11, r12, sp}^
    mov lr, r12

    mov r0, #0
    bx lr

