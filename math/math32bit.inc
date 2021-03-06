MATH_32BIT = 0

.ifndef ATMEGA328P_CONSTANTS
.include "atmega328p_core/const_codes.inc"
.endif

.text

; Function recieve 16bit value in r23:r22 registers and extends it sing bit up to 32bit value
; r23:r22 - velue to extends
; r23 - MSB
; r22 - LSB
; Return 32bit value with extended sign in r25:r22 registers
; r25 - MSB
; r22 - LSB
sign_extend_uint_32bit:
	clr r24
	clr r25
	
	sbrs r23, 0x07									; skip next instruction if sign bit is set
	rjmp _sign_extend_uint_32bit_exit
	
	; load r25 and r24 registers with sign bit
	ldi r24, 0xFF
	ldi r25, 0xFF
	
_sign_extend_uint_32bit_exit:
	ret

; Perform addition of 2 32bit values
; value1 - r25:r22 dividend
; r25 - MSB
; r22 - LSB
; value2 - r29:r26 divider
; r29 - MSB
; r26 - LSB
; function return result in r25:r22 registers
; r25:r22 - integer result of addition
add_uint_32bit:
	add r22, r26
	adc r23, r27
	adc r24, r28
	adc r25, r29
	ret

; do 32 bit unsigned integer division
; value1 - r25:r22 dividend
; r25 - MSB
; r22 - LSB
; value2 - r29:r26 divider
; r29 - MSB
; r26 - LSB
; function return result as 2 32bit values in r25:r22 and r29:r26 registers
; r25:r22 - integer result of division
; r29:r26 - reminder of division 
div_uint_32bit:
	push r16						; save state of temporary register
	; save state of Z register
	push r30
	push r31
	
	; set base pointer to Z register
	in r31, SPH
	in r30, SPL
	
	; save dividend to the stack
	push r25		; [base] 		MSB
	push r24		; [base + 0x01]
	push r23		; [base + 0x02]
	push r22		; [base + 0x03] LSB
	
	; save divider to the stack
	push r29		; [base + 0x04] MSB
	push r28		; [base + 0x05]
	push r27		; [base + 0x06]
	push r26		; [base + 0x07] LSB
	
	; reserve space in the stack for other values
	sbiw r30, 0x1A	; 8 bytes already pushed and 18 bytes should be reserved
	out SPH, r31
	out SPL, r30
	adiw r30, 0x1A
	
	; [base + 0x08] - result MSB
	; [base + 0x09]
	; [base + 0x0A]
	; [base + 0x0B] - result LSB
	; [base + 0x0C] - reminder MSB
	; [base + 0x0D]
	; [base + 0x0E]
	; [base + 0x0F]	- reminder LSB
	; [base + 0x10]	- temporary value of leading zeroes in dividend
	; [base + 0x11] - temporary value of leading zeroes in divider
	; [base + 0x12] - intermediate value MSB
	; [base + 0x13]
	; [base + 0x14]
	; [base + 0x15]	- intermediate value LSB 
	; [base + 0x16] - shifted divider MSB
	; [base + 0x17]
	; [base + 0x18]
	; [base + 0x19] - shifted divider LSB
	
	; initial reminder with dividend value (NB! should be before any changes appears in r25:r22 registers)
	sbiw r30, 0x0C
	st Z, r25
	sbiw r30, 0x01
	st Z, r24
	sbiw r30, 0x01
	st Z, r23
	sbiw r30, 0x01
	st Z, r22
	adiw r30, 0x0F
	
	; initialize intermediate value with dividend
	sbiw r30, 0x12
	st Z, r25
	sbiw r30, 0x01
	st Z, r24
	sbiw r30, 0x01
	st Z, r23
	sbiw r30, 0x01
	st Z, r22
	adiw r30, 0x15
	
	; initialize result with 0
	clr r24
	sbiw r30, 0x08
	st Z, r24
	sbiw r30, 0x01
	st Z, r24
	sbiw r30, 0x01
	st Z, r24
	sbiw r30, 0x01
	st Z, r24
	adiw r30, 0x0B
	
	; crear temporary value of leading zeroes
	sbiw r30, 0x10
	st Z, r24
	sbiw r30, 0x01
	st Z, r24
	adiw r30, 0x11
	
	; logic start
	
	; count leading zeroes in dividend
	ldi r25, 0x04						; set counter to size of dividend
	clr r26 							; r26 register will be used as displacement for next 8bit element
	clr r27								; used o add carry flag only to r31 register
	
_div_uint_32bit_count_dividend_zeroes:
	clc									; clear carry bit because should add displacement register to 16 bit register
	
	sub r30, r26
	sbc r31, r27
	
	ld r24, Z
	
	clc
	add r30, r26
	adc r31, r27
	
	rcall count_leading_zeroes_8bit
	
	

	sbiw r30, 0x10
	ld r16, Z
	add r16, r24
	st Z, r16
	adiw r30, 0x10
	
	; if found zeroes less then 8, should exit from this loop
	cpi r24, 0x08
	brne _div_uint_32bit_count_divider_zeroes_init
	
	inc r26	
	dec r25
	brne _div_uint_32bit_count_dividend_zeroes
	
_div_uint_32bit_count_divider_zeroes_init:
	; count leading zeroes in divider
	ldi r25, 0x04						; set counter to size of dividend
	ldi r26, 0x04						; r26 register will be used as displacement for next 8bit element (0x04 is initial displacement for divider)
	clr r27								; used o add carry flag only to r31 register
	
_div_uint_32bit_count_divider_zeroes:
	clc									; clear carry bit because should add displacement register to 16 bit register
	
	sub r30, r26
	sbc r31, r27
	
	ld r24, Z
	
	clc
	add r30, r26
	adc r31, r27
	
	rcall count_leading_zeroes_8bit

	sbiw r30, 0x11
	ld r16, Z
	add r16, r24
	st Z, r16
	adiw r30, 0x11
	
	; if found zeroes less then 8, should exit from this loop
	cpi r24, 0x08
	brne _div_uint_32bit_count_subtraction
	
	inc r26
	dec r25
	brne _div_uint_32bit_count_divider_zeroes
	
_div_uint_32bit_count_subtraction:
	; subtract zero bits in dividend from zero bits in divider
	sbiw r30, 0x10
	ld r24, Z
	sbiw r30, 0x01
	ld r25, Z
	adiw r30, 0x11
	
	sub r25, r24							; because I'm counting zeroes, I should subtract leading zeroes in dividend from leading zeroes in divider to get correct result
	
	sbrc r25, 0x07							; if sign bit is set, than exit					
	rjmp _div_uint_32bit_exit				; if leading zeroes in dividend more that in divider I should exit with 0 in result and dividend in reminder
	
	; save counter subtraction result to [base + 0x10]
	sbiw r30, 0x10
	st Z, r25
	adiw r30, 0x10
	
	; restore divider [base + 0x04] from stack to r29:r26 registers
	sbiw r30, 0x04
	ld r29, Z
	sbiw r30, 0x01
	ld r28, Z
	sbiw r30, 0x01
	ld r27, Z
	sbiw r30, 0x01
	ld r26, Z
	adiw r30, 0x07
	
	; load zero bit counters subtraction result to r16 register
	sbiw r30, 0x10
	ld r16, Z
	adiw r30, 0x10
	
_div_uint_32bit_shift_divider_loop:
	; shift divider left to number of bits in zero bit counter
	clc													; clear carry flag
	rol r26												; shift bits to the left 7th bit goes to Carry, Carry bit goes to 0th bit
	rol r27		
	rol r28
	rol r29
			
	dec r16												; decrement zero counter, so it will show exact number of zeroes before first 1
	brne _div_uint_32bit_shift_divider_loop
	
	; store shifted divider to the stack
	sbiw r30, 0x16
	st Z, r29
	sbiw r30, 0x01
	st Z, r28
	sbiw r30, 0x01
	st Z, r27
	sbiw r30, 0x01
	st Z, r26
	adiw r30, 0x19
	
_div_uint_32bit_division_begining_label:
	; store value from previous step to reminder [base + 0x0C]
	sbiw r30, 0x0C
	st Z, r29
	sbiw r30, 0x01
	st Z, r28
	sbiw r30, 0x01
	st Z, r27
	sbiw r30, 0x01
	st Z, r26
	adiw r30, 0x0F
	
	; load zero bit counters subtraction result to r16 register
	sbiw r30, 0x10
	ld r16, Z
	adiw r30, 0x10
							
	inc r16								; increment zeroes counter by 1, because subtraction should be done (zero counter + 1) times
	
_div_uint_32bit_division_main_loop:	
	; load result [base + 0x08] from the stack
	sbiw r30, 0x08
	ld r29, Z
	sbiw r30, 0x01
	ld r28, Z
	sbiw r30, 0x01
	ld r27, Z
	sbiw r30, 0x01
	ld r26, Z
	adiw r30, 0x0B
	
	; shift result left
	clc
	rol r26
	rol r27
	rol r28
	rol r29
	
	; dbg result -->
	;push r24
	;mov r24, r29
	;rcall send_to_usart
	;mov r24, r28
	;rcall send_to_usart
	;mov r24, r27
	;rcall send_to_usart
	;mov r24, r26
	;rcall send_to_usart
	;pop r24
	; dbg <--
	
	; store shifted result to the stack
	sbiw r30, 0x08
	st Z, r29
	sbiw r30, 0x01
	st Z, r28
	sbiw r30, 0x01
	st Z, r27
	sbiw r30, 0x01
	st Z, r26
	adiw r30, 0x0B
	
	; load shifted divider from the stack
	sbiw r30, 0x16
	ld r29, Z
	sbiw r30, 0x01
	ld r28, Z
	sbiw r30, 0x01
	ld r27, Z
	sbiw r30, 0x01
	ld r26, Z
	adiw r30, 0x19
	
	; restore intermediate result from stack [base + 0x12] to r25:r22
	sbiw r30, 0x12
	ld r25, Z
	sbiw r30, 0x01
	ld r24, Z
	sbiw r30, 0x01
	ld r23, Z
	sbiw r30, 0x01
	ld r22, Z
	adiw r30, 0x15
	
	; subtract shifted divider from intermediate result
	clc													; clear Carry flag
	sub r22, r26
	sbc r23, r27
	sbc r24, r28
	sbc r25, r29
	
	;and r22, r22										; needed to set Sign flag is any
	and r25, r25
	brmi _div_uint_32bit_division_main_loop_continue	; branch if result negative
	
	; if result >= 0
	
	; save substraction result to intermediate result [base + 0x12]
	sbiw r30, 0x12
	st Z, r25
	sbiw r30, 0x01
	st Z, r24
	sbiw r30, 0x01
	st Z, r23
	sbiw r30, 0x01
	st Z, r22
	adiw r30, 0x15
	
	; set appropriate bit in to result (result is rotating during the loop, so bit should be always set to position 0)
	push r16
	sbiw r30, 0x0B
	ld r16, Z	
	ori r16, 0x01
	st Z, r16
	adiw r30, 0x0B
	pop r16
	
	; save subtraction result to reminder [base + 0x0C] in stack
	sbiw r30, 0x0C
	st Z, r25
	sbiw r30, 0x01
	st Z, r24
	sbiw r30, 0x01
	st Z, r23
	sbiw r30, 0x01
	st Z, r22
	adiw r30, 0x0F
	
_div_uint_32bit_division_main_loop_continue:
	dec r16												; decrement zero counter
	breq _div_uint_32bit_exit
	
	; shit divider rigth
	clc
	ror r29
	ror r28
	ror r27
	ror r26
	
	; store shifted divider to the stack
	sbiw r30, 0x16
	st Z, r29
	sbiw r30, 0x01
	st Z, r28
	sbiw r30, 0x01
	st Z, r27
	sbiw r30, 0x01
	st Z, r26
	adiw r30, 0x19
	
	rjmp _div_uint_32bit_division_main_loop
	
_div_uint_32bit_exit:

	; move result to r25:r22 registers
	sbiw r30, 0x08
	ld r25, Z
	sbiw r30, 0x01
	ld r24, Z
	sbiw r30, 0x01
	ld r23, Z
	sbiw r30, 0x01
	ld r22, Z
	adiw r30, 0x0B
	
	; move reminder to r29:r26 register
	sbiw r30, 0x0C
	ld r29, Z
	sbiw r30, 0x01
	ld r28, Z
	sbiw r30, 0x01
	ld r27, Z
	sbiw r30, 0x01
	ld r26, Z
	adiw r30, 0x0F
	
	; dbg
	;push r24
	;mov r24, r31
	;rcall send_to_usart
	;mov r24, r30
	;rcall send_to_usart
	;pop r24
	; dbg

	; restore stack pointer from base pointer
	out SPH, r31
	out SPL, r30
	
	pop r31
	pop r30
	
	pop r16
	
	ret

; Invert sign of 32 bit value
; Value recieved in r27:r24 registers
; r27 - MSB
; r24 - LSB
; Value will be returned in r27:r24 registers
; r27 - MSB
; r24 - LSB
invert_sign_32bit:
	; save state of temporary register
	push r16
	
	; check does recieved value positive
	sbrc r27, 0x07		; check does 7th bit is cleare, if so skips next instruction
	rjmp _invert_sign_32bit_set_positive
	
	; change sign to negative
	; step 1 change invert registers bits
	ldi r16, 0xFF
	eor r27, r16
	eor r26, r16
	eor r25, r16
	eor r24, r16
	
	; add to inverted registers 0x01
	ldi r16, 0x01
	
	clc			; clear carry flag
	
	add r24, r16
	clr r16
	adc r25, r16	; add carry bit if any
	adc r26, r16	; add carry bit if any
	adc r27, r16	; add carry bit if any
	
	rjmp _invert_sign_32bit_exit
	
_invert_sign_32bit_set_positive:
	; change sign to positive
	
	; subtract 0x01 from recieved value
	ldi r16, 0x01
	
	clc		; clear carry flag
	
	sub r24, r16
	clr r16
	sbc r25, r16		; subtract carry flag if any
	sbc r26, r16		; subtract carry flag if any
	sbc r27, r16		; subtract carry flag if any
	
	; invert registers bits
	ldi r16, 0xFF
	eor r27, r16
	eor r26, r16
	eor r25, r16
	eor r24, r16
	
_invert_sign_32bit_exit:
	
	pop r16	
	
	ret
