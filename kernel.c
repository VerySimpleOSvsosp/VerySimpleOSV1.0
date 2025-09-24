#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint32_t eax, ebx, ecx, edx, esi, edi, ebp, esp, eip, eflags;
    uint32_t cr3, cr0;
} cpu_state_t;

typedef struct {
    uint32_t pid;
    uint32_t esp;
    uint32_t stack_base;
    cpu_state_t state;
} process_t;

process_t *current_process;
process_t processes[10];
uint32_t next_pid = 0;

extern void enable_interrupts();
extern void disable_interrupts();
extern void switch_context(uint32_t esp);
extern void outb(uint16_t port, uint8_t value);
extern uint8_t inb(uint16_t port);

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
volatile uint16_t* vga_buffer = (uint16_t*)0xB8000;
uint8_t terminal_color = 0x07;
uint8_t terminal_row = 0;
uint8_t terminal_column = 0;

void terminal_initialize() {
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            vga_buffer[index] = (uint16_t)' ' | (uint16_t)terminal_color << 8;
        }
    }
    terminal_row = 0;
    terminal_column = 0;
}

void terminal_setcolor(uint8_t color) {
    terminal_color = color;
}

void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT) {
            terminal_row = 0;
        }
        return;
    }
    
    const size_t index = terminal_row * VGA_WIDTH + terminal_column;
    vga_buffer[index] = (uint16_t)c | (uint16_t)terminal_color << 8;
    
    if (++terminal_column == VGA_WIDTH) {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT) {
            terminal_row = 0;
        }
    }
}

void terminal_writestring(const char* data) {
    for (size_t i = 0; data[i] != '\0'; i++) {
        terminal_putchar(data[i]);
    }
}

void terminal_writehex(uint32_t num) {
    const char* hex = "0123456789ABCDEF";
    terminal_writestring("0x");
    
    for (int i = 7; i >= 0; i--) {
        uint8_t nibble = (num >> (i * 4)) & 0xF;
        terminal_putchar(hex[nibble]);
    }
}

void* malloc(size_t size) {
    static uint8_t memory_pool[1024 * 64];
    static size_t next_free = 0;
    
    if (next_free + size > sizeof(memory_pool)) {
        return NULL;
    }
    
    void* ptr = &memory_pool[next_free];
    next_free += size;
    return ptr;
}

void scheduler_init() {
    processes[0].pid = next_pid++;
    processes[0].stack_base = 0;
    current_process = &processes[0];
}

uint32_t create_process(void (*entry)()) {
    disable_interrupts();
    
    int i;
    for (i = 1; i < 10; i++) {
        if (processes[i].pid == 0) break;
    }
    
    if (i == 10) return 0;
    
    uint32_t stack = (uint32_t)malloc(16384) + 16384;
    
    processes[i].pid = next_pid++;
    processes[i].stack_base = stack - 16384;
    processes[i].esp = stack;
    
    processes[i].state.esp = stack;
    processes[i].state.eip = (uint32_t)entry;
    processes[i].state.eflags = 0x202;
    
    enable_interrupts();
    return processes[i].pid;
}

void schedule() {
    static int current = 0;
    
    processes[current].esp = current_process->esp;
    
    int next = (current + 1) % 10;
    while (processes[next].pid == 0) {
        next = (next + 1) % 10;
    }
    
    current = next;
    current_process = &processes[current];
    switch_context(current_process->esp);
}

void syscall_handler() {
    __asm__ volatile("mov %%esp, %0" : "=r"(current_process->esp));
}

void gdt_init();
void idt_init();

void kernel_main() {
    disable_interrupts();
    gdt_init();
    idt_init();
    scheduler_init();
    
    terminal_initialize();
    terminal_writestring("32-bit Microkernel Booted Successfully!\n");
    terminal_writestring("Hello World from 32-bit Microkernel!\n");
    terminal_writestring("Kernel loaded at: ");
    terminal_writehex((uint32_t)&kernel_main);
    terminal_putchar('\n');
    
    enable_interrupts();
    
    while (1) {
        __asm__ volatile("hlt");
        schedule();
    }
}
