
#include <stdint.h>
#include <malloc.h>
#include <assert.h>
#include <io.h>

struct phys_page {
	paddr_t addr;
	struct phys_page *next;
};

/*@null@*/ static struct phys_page * volatile page_stack = 0;

paddr_t pmem_alloc() {
	struct phys_page *page = 0;
	paddr_t ret = 0;

	if(page_stack == 0)
		return 0;
	assert(page_stack->addr != 0);

	// Remove from the top of the stack.
	/// \todo Atomicity.
	page = page_stack;
	page_stack = page_stack->next;
	ret = page->addr;
	free(page);

	return ret;
}

void pmem_dealloc(paddr_t p) {
	struct phys_page *page = 0;

	page = (struct phys_page *) malloc(sizeof(struct phys_page));
	page->addr = p;

	/// \todo Atomicity
	page->next = page_stack;
	page_stack = page;
}

void pmem_pin(paddr_t p) {
	struct phys_page *page = page_stack, *prev = 0;
	if(page_stack == 0)
		return;

	while(page && (page->addr != p)) {
		prev = page;
		page = page->next;
	}

	if(!page)
		return; // Page isn't in the stack.

	// Make un-allocateable.
	prev->next = page->next;
	free(page);
}

