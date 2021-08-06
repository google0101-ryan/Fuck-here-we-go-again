[global gdt_flush]
gdt_flush:
    mov eax, [esp+4]
    lgdt [eax]

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp 0x8:flush
flush:
    ret

[global idt_flush]    ; Allows the C code to call idt_flush().
idt_flush:
    mov eax, [esp+4]  ; Get the pointer to the IDT, passed as a parameter. 
    lidt [eax]        ; Load the IDT pointer.
    ret


[global tss_flush]   ; Allows our C code to call tss_flush().
tss_flush:
	mov ax, 0x2B      ; Load the index of our TSS structure - The index is
										; 0x28, as it is the 5th selector and each is 8 bytes
										; long, but we set the bottom two bits (making 0x2B)
										; so that it has an RPL of 3, not zero.
	ltr ax            ; Load 0x2B into the task state register.
	ret