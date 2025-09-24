
boot.o: boot.asm
	nasm -f elf32 boot.asm -o boot.o

gdt.o: gdt.asm
	nasm -f elf32 gdt.asm -o gdt.o

idt.o: idt.asm
	nasm -f elf32 idt.asm -o idt.o

kernel.o: kernel.c
	gcc -m32 -c kernel.c -o kernel.o -ffreestanding -nostdlib

gdt_c.o: gdt.c
	gcc -m32 -c gdt.c -o gdt_c.o -ffreestanding -nostdlib

idt_c.o: idt.c
	gcc -m32 -c idt.c -o idt_c.o -ffreestanding -nostdlib

kernel.bin: boot.o kernel.o gdt.o gdt_c.o idt.o idt_c.o
	ld -m elf_i386 -T linker.ld -o kernel.bin boot.o kernel.o gdt.o gdt_c.o idt.o idt_c.o

iso: kernel.bin
	mkdir -p isodir/boot/grub
	cp kernel.bin isodir/boot/
	echo 'menuentry "32-bit Microkernel" { multiboot /boot/kernel.bin }' > isodir/boot/grub/grub.cfg
	grub-mkrescue -o myos.iso isodir

clean:
	rm -f *.o *.bin *.iso
	rm -rf isodir
