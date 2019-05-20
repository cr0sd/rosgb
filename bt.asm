;nasm bt.asm -f win32
global _bittest
section .code

; bittest(uint8_t a,uint8_t b,uint8_t bit)
; Returns ( bit(a,bit) | ( bit(b,bit) <<1 ) )
; for bit 'bit'
_bittest:
	push ebp
	mov ebp,esp
	
	mov eax,[ebp+8]
	mov ebx,[ebp+12]
	mov edi,[ebp+16]
	xor edx,edx
	mov ecx,1
	
.testa:
	bt eax,edi ;bit(a,0)
	jnc .testb
	mov edx,1
	
.testb:
	bt ebx,edi ;bit(b,0)
	jnc .testbfalse
	mov eax,1
	jmp .end
	
.testbfalse:
	xor eax,eax
.end:
	shl eax,1
	or edx,eax
	mov eax,edx
	
	mov esp,ebp
	pop ebp
	ret