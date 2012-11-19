;-----------------------------------
; doublewide.asm
;
; doubleWideAsm - duplicate pixels from byte to word

	.global doubleWideAsm

;	R12 input character byte
; 	R13 input linebuff address to receive word
;	R14 temp for output word
;	R15 temp for bit count

doubleWideAsm:
		MOV.B	#8, R15		; set bit counter to 8
nextPixel:
		RRA.B	R12			; shift pixel into carry
		RRC.W	R14			; shift pixel into word
		RRA.W	R14			; duplicate pixel
		DEC.B	R15			; decrease bit counter
		JNZ nextPixel		; not done yet
		MOV.B	R14, 1(R13) ; store 1st byte in linebuff	not using MOV.W as linebuff position might be odd
		SWPB	R14			; get access to 2nd byte
		MOV.B	R14, 0(R13)	; store 2nd byte in linebuff
		RET					; done
