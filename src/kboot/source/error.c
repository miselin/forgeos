/*
 * Copyright (C) 2010-2011 Alex Smith
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/**
 * @file
 * @brief		Boot error handling functions.
 */

#include <lib/printf.h>

#include <memory.h>
#include <system.h>
#include <ui.h>

#if CONFIG_KBOOT_UI
/** Boot error window state. */
static const char *boot_error_format;
static va_list boot_error_args;
static ui_window_t *debug_log_window;
#endif

/** Helper for internal_error_printf().
 * @param ch		Character to display.
 * @param data		If not NULL, newlines will be padded.
 * @param total		Pointer to total character count. */
static void internal_error_printf_helper(char ch, void *data, int *total) {
	if(debug_console) {
		debug_console->putch(ch);
	}
	if(main_console) {
		main_console->putch(ch);
	}
	*total = *total + 1;
}

/** Formatted print function for internal_error(). */
static int internal_error_printf(const char *fmt, ...) {
	va_list args;
	int ret;

	va_start(args, fmt);
	ret = do_printf(internal_error_printf_helper, NULL, fmt, args);
	va_end(args);

	return ret;
}

/** Raise an internal error.
 * @param fmt		Error format string.
 * @param ...		Values to substitute into format. */
void __noreturn internal_error(const char *fmt, ...) {
	va_list args;

	if(main_console) {
		main_console->reset();
	}
	internal_error_printf("\nAn internal error has occurred:\n\n");

	va_start(args, fmt);
	do_printf(internal_error_printf_helper, NULL, fmt, args);
	va_end(args);

	internal_error_printf("\n\n");
	internal_error_printf("Please report this error to http://kiwi.alex-smith.me.uk/\n");
	internal_error_printf("Backtrace:\n");
	backtrace(internal_error_printf);
	while(1);
}

/** Print the boot error message. */
static void boot_error_display(const char *fmt, va_list args) {
	internal_error_printf("An error has occurred during boot:\n\n");

	do_printf(internal_error_printf_helper, NULL, fmt, args);

	internal_error_printf("\n\n");
	internal_error_printf("Ensure that you have enough memory available, that you do not have any\n");
	internal_error_printf("malfunctioning hardware and that your computer meets the minimum system\n");
	internal_error_printf("requirements for the operating system.\n");
}

#if CONFIG_KBOOT_UI
/** Render the boot error window.
 * @param window	Window to render. */
static void boot_error_window_render(ui_window_t *window) {
	boot_error_display(boot_error_format, boot_error_args);
}

/** Write the help text for the boot error window.
 * @param window	Window to write for. */
static void boot_error_window_help(ui_window_t *window) {
	kprintf("F1 = Debug Log  Esc = Reboot");
}

/** Handle input on the boot error window.
 * @param window	Window input was performed on.
 * @param key		Key that was pressed.
 * @return		Input handling result. */
static input_result_t boot_error_window_input(ui_window_t *window, uint16_t key) {
	switch(key) {
	case CONSOLE_KEY_F1:
		ui_window_display(debug_log_window, 0);
		return INPUT_RENDER;
	case '\e':
		platform_reboot();
		__attribute__ ((fallthrough));
	default:
		return INPUT_HANDLED;
	}
}

/** Boot error window type. */
static ui_window_type_t boot_error_window_type = {
	.render = boot_error_window_render,
	.help = boot_error_window_help,
	.input = boot_error_window_input,
};
#endif

/** Display details of a boot error.
 * @param fmt		Error format string.
 * @param ...		Values to substitute into format. */
void __noreturn boot_error(const char *fmt, ...) {
#if CONFIG_KBOOT_UI
	ui_window_t *window;

	boot_error_format = fmt;
	va_start(boot_error_args, fmt);

	/* Create the debug log window. */
	debug_log_window = ui_textview_create("Debug Log", debug_log);

	/* Create the error window and display it. */
	window = kmalloc(sizeof(ui_window_t));
	ui_window_init(window, &boot_error_window_type, "Boot Error");
	ui_window_display(window, 0);
#else
	va_list args;

	if(main_console) {
		main_console->reset();
	}
	if(debug_console) {
		debug_console->putch('\n');
	}

	va_start(args, fmt);
	boot_error_display(fmt, args);
	va_end(args);
#endif
	while(1);
}
