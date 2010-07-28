
#include <in_asm.h>
#include <assembler.h>

/**	EXPLANATION:
 * Define all int vectors here. On x86-32 we have auto-stack switching. If we
 * didn't, we'd have to use one of the debug registers to hold stack context,
 * and then pull up the thread's kernel stack, and load the stack pointer.
 *
 * All we do as of now is:
 *	1. Push all GP regs onto our kernel stack.
 *	2. Push all data segment regs.
 *	3. Move into the kernel via interruptTrib_irqEntry().
 **/

ASM_LOCAL_FUNCTION(__ksaveRegs)
	pusha

	/* EAX = (%ds << $16) | %es ; EBX = (%fs << 16) | %gs */
	movw	%fs, %bx
	shl	$16, %ebx
	movw	%gs, %bx
	movw	%ds, %ax
	shl	$16, %eax
	movl	%es, %ax

	/* Push regs: FS|GS then DS|ES. From C, this will look like DS|ES
	 * comes first on the stack.
	 **/
	pushl	%ebx
	pushl	%eax

	movw	$0x10, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs

	/* Now have all of the kernel segments loaded. Context is on stack.
	 * Push the address of the context, and call into the kernel.
	 **/
	pushl	%esp
	call	interruptTrib_irqEntry

	/* From a technical POV, Zambezii never should reach here. When we
	 * return from a interrupt context, we simply point the stack pointer
	 * to the context structure and pop and ret, all from within inline ASM
	 * in C.
	 *
	 * Reaching here is a bad thing(TM) in that sense. But by no means
	 * should our kernel crash simply because of that. We should then just
	 * do the normal thing and then IRET.
	 **/

	popl	%eax
	popl	%ebx

	/* Reload whatever userspace had in its segment regs. */
	movw	%ax, %es
	shr	$16, %eax
	movw	%ax, %ds
	movw	%bx, %gs
	shr	$16, %ebx
	movw	%bx, %fs

	popa
	addl	$0x8, %esp
iret
ASM_END_FUNCTION(__ksaveRegs)

/**	EXLPLANATION:
 * Vector handlers: they push the vector number, and, depending on the vector,
 * a value to even off the stack.
 **/

ASM_GLOBAL_FUNCTION(vector0)
	pushl $0
	pushl $0
	jmp __ksaveRegs
ASM_END_FUNCTION(vector0)

ASM_GLOBAL_FUNCTION(vector1)
	pushl $0
	pushl $1
	jmp __ksaveRegs
ASM_END_FUNCTION(vector1)

ASM_GLOBAL_FUNCTION(vector2)
	pushl $0
	pushl $2
	jmp __ksaveRegs
ASM_END_FUNCTION(vector2)

ASM_GLOBAL_FUNCTION(vector3)
	pushl $0
	pushl $3
	jmp __ksaveRegs
ASM_END_FUNCTION(vector3)

ASM_GLOBAL_FUNCTION(vector4)
	pushl $0
	pushl $4
	jmp __ksaveRegs
ASM_END_FUNCTION(vector4)

ASM_GLOBAL_FUNCTION(vector5)
	pushl $0
	pushl $5
	jmp __ksaveRegs
ASM_END_FUNCTION(vector5)

ASM_GLOBAL_FUNCTION(vector6)
	pushl $0
	pushl $6
	jmp __ksaveRegs
ASM_END_FUNCTION(vector6)

ASM_GLOBAL_FUNCTION(vector7)
	pushl $0
	pushl $7
	jmp __ksaveRegs
ASM_END_FUNCTION(vector7)

ASM_GLOBAL_FUNCTION(vector8)
	pushl $8
	jmp __ksaveRegs
ASM_END_FUNCTION(vector8)

ASM_GLOBAL_FUNCTION(vector9)
	pushl $0
	pushl $9
	jmp __ksaveRegs
ASM_END_FUNCTION(vector9)

ASM_GLOBAL_FUNCTION(vector10)
	pushl $10
	jmp __ksaveRegs
ASM_END_FUNCTION(vector10)

ASM_GLOBAL_FUNCTION(vector11)
	pushl $11
	jmp __ksaveRegs
ASM_END_FUNCTION(vector11)

ASM_GLOBAL_FUNCTION(vector12)
	pushl $12
	jmp __ksaveRegs
ASM_END_FUNCTION(vector12)

ASM_GLOBAL_FUNCTION(vector13)
	pushl $13
	jmp __ksaveRegs
ASM_END_FUNCTION(vector13)

ASM_GLOBAL_FUNCTION(vector14)
	pushl $14
	jmp __ksaveRegs
ASM_END_FUNCTION(vector14)

ASM_GLOBAL_FUNCTION(vector15)
	pushl $0
	pushl $15
	jmp __ksaveRegs
ASM_END_FUNCTION(vector15)

ASM_GLOBAL_FUNCTION(vector16)
	pushl $0
	pushl $16
	jmp __ksaveRegs
ASM_END_FUNCTION(vector16)

ASM_GLOBAL_FUNCTION(vector17)
	pushl $0
	pushl $17
	jmp __ksaveRegs
ASM_END_FUNCTION(vector17)

ASM_GLOBAL_FUNCTION(vector18)
	pushl $0
	pushl $18
	jmp __ksaveRegs
ASM_END_FUNCTION(vector18)

ASM_GLOBAL_FUNCTION(vector19)
	pushl $0
	pushl $19
	jmp __ksaveRegs
ASM_END_FUNCTION(vector19)

ASM_GLOBAL_FUNCTION(vector20)
	pushl $0
	pushl $20
	jmp __ksaveRegs
ASM_END_FUNCTION(vector20)

ASM_GLOBAL_FUNCTION(vector21)
	pushl $0
	pushl $21
	jmp __ksaveRegs
ASM_END_FUNCTION(vector21)

ASM_GLOBAL_FUNCTION(vector22)
	pushl $0
	pushl $22
	jmp __ksaveRegs
ASM_END_FUNCTION(vector22)

ASM_GLOBAL_FUNCTION(vector23)
	pushl $0
	pushl $23
	jmp __ksaveRegs
ASM_END_FUNCTION(vector23)

ASM_GLOBAL_FUNCTION(vector24)
	pushl $0
	pushl $24
	jmp __ksaveRegs
ASM_END_FUNCTION(vector24)

ASM_GLOBAL_FUNCTION(vector25)
	pushl $0
	pushl $25
	jmp __ksaveRegs
ASM_END_FUNCTION(vector25)

ASM_GLOBAL_FUNCTION(vector26)
	pushl $0
	pushl $26
	jmp __ksaveRegs
ASM_END_FUNCTION(vector26)

ASM_GLOBAL_FUNCTION(vector27)
	pushl $0
	pushl $27
	jmp __ksaveRegs
ASM_END_FUNCTION(vector27)

ASM_GLOBAL_FUNCTION(vector28)
	pushl $0
	pushl $28
	jmp __ksaveRegs
ASM_END_FUNCTION(vector28)

ASM_GLOBAL_FUNCTION(vector29)
	pushl $0
	pushl $29
	jmp __ksaveRegs
ASM_END_FUNCTION(vector29)

ASM_GLOBAL_FUNCTION(vector30)
	pushl $0
	pushl $30
	jmp __ksaveRegs
ASM_END_FUNCTION(vector30)

ASM_GLOBAL_FUNCTION(vector31)
	pushl $0
	pushl $31
	jmp __ksaveRegs
ASM_END_FUNCTION(vector31)

ASM_GLOBAL_FUNCTION(vector32)
	pushl $0
	pushl $32
	jmp __ksaveRegs
ASM_END_FUNCTION(vector32)

ASM_GLOBAL_FUNCTION(vector33)
	pushl $0
	pushl $33
	jmp __ksaveRegs
ASM_END_FUNCTION(vector33)

ASM_GLOBAL_FUNCTION(vector34)
	pushl $0
	pushl $34
	jmp __ksaveRegs
ASM_END_FUNCTION(vector34)

ASM_GLOBAL_FUNCTION(vector35)
	pushl $0
	pushl $35
	jmp __ksaveRegs
ASM_END_FUNCTION(vector35)

ASM_GLOBAL_FUNCTION(vector36)
	pushl $0
	pushl $36
	jmp __ksaveRegs
ASM_END_FUNCTION(vector36)

ASM_GLOBAL_FUNCTION(vector37)
	pushl $0
	pushl $37
	jmp __ksaveRegs
ASM_END_FUNCTION(vector37)

ASM_GLOBAL_FUNCTION(vector38)
	pushl $0
	pushl $38
	jmp __ksaveRegs
ASM_END_FUNCTION(vector38)

ASM_GLOBAL_FUNCTION(vector39)
	pushl $0
	pushl $39
	jmp __ksaveRegs
ASM_END_FUNCTION(vector39)

ASM_GLOBAL_FUNCTION(vector40)
	pushl $0
	pushl $40
	jmp __ksaveRegs
ASM_END_FUNCTION(vector40)

ASM_GLOBAL_FUNCTION(vector41)
	pushl $0
	pushl $41
	jmp __ksaveRegs
ASM_END_FUNCTION(vector41)

ASM_GLOBAL_FUNCTION(vector42)
	pushl $0
	pushl $42
	jmp __ksaveRegs
ASM_END_FUNCTION(vector42)

ASM_GLOBAL_FUNCTION(vector43)
	pushl $0
	pushl $43
	jmp __ksaveRegs
ASM_END_FUNCTION(vector43)

ASM_GLOBAL_FUNCTION(vector44)
	pushl $0
	pushl $44
	jmp __ksaveRegs
ASM_END_FUNCTION(vector44)

ASM_GLOBAL_FUNCTION(vector45)
	pushl $0
	pushl $45
	jmp __ksaveRegs
ASM_END_FUNCTION(vector45)

ASM_GLOBAL_FUNCTION(vector46)
	pushl $0
	pushl $46
	jmp __ksaveRegs
ASM_END_FUNCTION(vector46)

ASM_GLOBAL_FUNCTION(vector47)
	pushl $0
	pushl $47
	jmp __ksaveRegs
ASM_END_FUNCTION(vector47)

ASM_GLOBAL_FUNCTION(vector48)
	pushl $0
	pushl $48
	jmp __ksaveRegs
ASM_END_FUNCTION(vector48)

ASM_GLOBAL_FUNCTION(vector49)
	pushl $0
	pushl $49
	jmp __ksaveRegs
ASM_END_FUNCTION(vector49)

ASM_GLOBAL_FUNCTION(vector50)
	pushl $0
	pushl $50
	jmp __ksaveRegs
ASM_END_FUNCTION(vector50)

ASM_GLOBAL_FUNCTION(vector51)
	pushl $0
	pushl $51
	jmp __ksaveRegs
ASM_END_FUNCTION(vector51)

ASM_GLOBAL_FUNCTION(vector52)
	pushl $0
	pushl $52
	jmp __ksaveRegs
ASM_END_FUNCTION(vector52)

ASM_GLOBAL_FUNCTION(vector53)
	pushl $0
	pushl $53
	jmp __ksaveRegs
ASM_END_FUNCTION(vector53)

ASM_GLOBAL_FUNCTION(vector54)
	pushl $0
	pushl $54
	jmp __ksaveRegs
ASM_END_FUNCTION(vector54)

ASM_GLOBAL_FUNCTION(vector55)
	pushl $0
	pushl $55
	jmp __ksaveRegs
ASM_END_FUNCTION(vector55)

ASM_GLOBAL_FUNCTION(vector56)
	pushl $0
	pushl $56
	jmp __ksaveRegs
ASM_END_FUNCTION(vector56)

ASM_GLOBAL_FUNCTION(vector57)
	pushl $0
	pushl $57
	jmp __ksaveRegs
ASM_END_FUNCTION(vector57)

ASM_GLOBAL_FUNCTION(vector58)
	pushl $0
	pushl $58
	jmp __ksaveRegs
ASM_END_FUNCTION(vector58)

ASM_GLOBAL_FUNCTION(vector59)
	pushl $0
	pushl $59
	jmp __ksaveRegs
ASM_END_FUNCTION(vector59)

ASM_GLOBAL_FUNCTION(vector60)
	pushl $0
	pushl $60
	jmp __ksaveRegs
ASM_END_FUNCTION(vector60)

ASM_GLOBAL_FUNCTION(vector61)
	pushl $0
	pushl $61
	jmp __ksaveRegs
ASM_END_FUNCTION(vector61)

ASM_GLOBAL_FUNCTION(vector62)
	pushl $0
	pushl $62
	jmp __ksaveRegs
ASM_END_FUNCTION(vector62)

ASM_GLOBAL_FUNCTION(vector63)
	pushl $0
	pushl $63
	jmp __ksaveRegs
ASM_END_FUNCTION(vector63)

ASM_GLOBAL_FUNCTION(vector64)
	pushl $0
	pushl $64
	jmp __ksaveRegs
ASM_END_FUNCTION(vector64)

ASM_GLOBAL_FUNCTION(vector65)
	pushl $0
	pushl $65
	jmp __ksaveRegs
ASM_END_FUNCTION(vector65)

ASM_GLOBAL_FUNCTION(vector66)
	pushl $0
	pushl $66
	jmp __ksaveRegs
ASM_END_FUNCTION(vector66)

ASM_GLOBAL_FUNCTION(vector67)
	pushl $0
	pushl $67
	jmp __ksaveRegs
ASM_END_FUNCTION(vector67)

ASM_GLOBAL_FUNCTION(vector68)
	pushl $0
	pushl $68
	jmp __ksaveRegs
ASM_END_FUNCTION(vector68)

ASM_GLOBAL_FUNCTION(vector69)
	pushl $0
	pushl $69
	jmp __ksaveRegs
ASM_END_FUNCTION(vector69)

ASM_GLOBAL_FUNCTION(vector70)
	pushl $0
	pushl $70
	jmp __ksaveRegs
ASM_END_FUNCTION(vector70)

ASM_GLOBAL_FUNCTION(vector71)
	pushl $0
	pushl $71
	jmp __ksaveRegs
ASM_END_FUNCTION(vector71)

ASM_GLOBAL_FUNCTION(vector72)
	pushl $0
	pushl $72
	jmp __ksaveRegs
ASM_END_FUNCTION(vector72)

ASM_GLOBAL_FUNCTION(vector73)
	pushl $0
	pushl $73
	jmp __ksaveRegs
ASM_END_FUNCTION(vector73)

ASM_GLOBAL_FUNCTION(vector74)
	pushl $0
	pushl $74
	jmp __ksaveRegs
ASM_END_FUNCTION(vector74)

ASM_GLOBAL_FUNCTION(vector75)
	pushl $0
	pushl $75
	jmp __ksaveRegs
ASM_END_FUNCTION(vector75)

ASM_GLOBAL_FUNCTION(vector76)
	pushl $0
	pushl $76
	jmp __ksaveRegs
ASM_END_FUNCTION(vector76)

ASM_GLOBAL_FUNCTION(vector77)
	pushl $0
	pushl $77
	jmp __ksaveRegs
ASM_END_FUNCTION(vector77)

ASM_GLOBAL_FUNCTION(vector78)
	pushl $0
	pushl $78
	jmp __ksaveRegs
ASM_END_FUNCTION(vector78)

ASM_GLOBAL_FUNCTION(vector79)
	pushl $0
	pushl $79
	jmp __ksaveRegs
ASM_END_FUNCTION(vector79)

ASM_GLOBAL_FUNCTION(vector80)
	pushl $0
	pushl $80
	jmp __ksaveRegs
ASM_END_FUNCTION(vector80)

ASM_GLOBAL_FUNCTION(vector81)
	pushl $0
	pushl $81
	jmp __ksaveRegs
ASM_END_FUNCTION(vector81)

ASM_GLOBAL_FUNCTION(vector82)
	pushl $0
	pushl $82
	jmp __ksaveRegs
ASM_END_FUNCTION(vector82)

ASM_GLOBAL_FUNCTION(vector83)
	pushl $0
	pushl $83
	jmp __ksaveRegs
ASM_END_FUNCTION(vector83)

ASM_GLOBAL_FUNCTION(vector84)
	pushl $0
	pushl $84
	jmp __ksaveRegs
ASM_END_FUNCTION(vector84)

ASM_GLOBAL_FUNCTION(vector85)
	pushl $0
	pushl $85
	jmp __ksaveRegs
ASM_END_FUNCTION(vector85)

ASM_GLOBAL_FUNCTION(vector86)
	pushl $0
	pushl $86
	jmp __ksaveRegs
ASM_END_FUNCTION(vector86)

ASM_GLOBAL_FUNCTION(vector87)
	pushl $0
	pushl $87
	jmp __ksaveRegs
ASM_END_FUNCTION(vector87)

ASM_GLOBAL_FUNCTION(vector88)
	pushl $0
	pushl $88
	jmp __ksaveRegs
ASM_END_FUNCTION(vector88)

ASM_GLOBAL_FUNCTION(vector89)
	pushl $0
	pushl $89
	jmp __ksaveRegs
ASM_END_FUNCTION(vector89)

ASM_GLOBAL_FUNCTION(vector90)
	pushl $0
	pushl $90
	jmp __ksaveRegs
ASM_END_FUNCTION(vector90)

ASM_GLOBAL_FUNCTION(vector91)
	pushl $0
	pushl $91
	jmp __ksaveRegs
ASM_END_FUNCTION(vector91)

ASM_GLOBAL_FUNCTION(vector92)
	pushl $0
	pushl $92
	jmp __ksaveRegs
ASM_END_FUNCTION(vector92)

ASM_GLOBAL_FUNCTION(vector93)
	pushl $0
	pushl $93
	jmp __ksaveRegs
ASM_END_FUNCTION(vector93)

ASM_GLOBAL_FUNCTION(vector94)
	pushl $0
	pushl $94
	jmp __ksaveRegs
ASM_END_FUNCTION(vector94)

ASM_GLOBAL_FUNCTION(vector95)
	pushl $0
	pushl $95
	jmp __ksaveRegs
ASM_END_FUNCTION(vector95)

ASM_GLOBAL_FUNCTION(vector96)
	pushl $0
	pushl $96
	jmp __ksaveRegs
ASM_END_FUNCTION(vector96)

ASM_GLOBAL_FUNCTION(vector97)
	pushl $0
	pushl $97
	jmp __ksaveRegs
ASM_END_FUNCTION(vector97)

ASM_GLOBAL_FUNCTION(vector98)
	pushl $0
	pushl $98
	jmp __ksaveRegs
ASM_END_FUNCTION(vector98)

ASM_GLOBAL_FUNCTION(vector99)
	pushl $0
	pushl $99
	jmp __ksaveRegs
ASM_END_FUNCTION(vector99)

ASM_GLOBAL_FUNCTION(vector100)
	pushl $0
	pushl $100
	jmp __ksaveRegs
ASM_END_FUNCTION(vector100)

ASM_GLOBAL_FUNCTION(vector101)
	pushl $0
	pushl $101
	jmp __ksaveRegs
ASM_END_FUNCTION(vector101)

ASM_GLOBAL_FUNCTION(vector102)
	pushl $0
	pushl $102
	jmp __ksaveRegs
ASM_END_FUNCTION(vector102)

ASM_GLOBAL_FUNCTION(vector103)
	pushl $0
	pushl $103
	jmp __ksaveRegs
ASM_END_FUNCTION(vector103)

ASM_GLOBAL_FUNCTION(vector104)
	pushl $0
	pushl $104
	jmp __ksaveRegs
ASM_END_FUNCTION(vector104)

ASM_GLOBAL_FUNCTION(vector105)
	pushl $0
	pushl $105
	jmp __ksaveRegs
ASM_END_FUNCTION(vector105)

ASM_GLOBAL_FUNCTION(vector106)
	pushl $0
	pushl $106
	jmp __ksaveRegs
ASM_END_FUNCTION(vector106)

ASM_GLOBAL_FUNCTION(vector107)
	pushl $0
	pushl $107
	jmp __ksaveRegs
ASM_END_FUNCTION(vector107)

ASM_GLOBAL_FUNCTION(vector108)
	pushl $0
	pushl $108
	jmp __ksaveRegs
ASM_END_FUNCTION(vector108)

ASM_GLOBAL_FUNCTION(vector109)
	pushl $0
	pushl $109
	jmp __ksaveRegs
ASM_END_FUNCTION(vector109)

ASM_GLOBAL_FUNCTION(vector110)
	pushl $0
	pushl $110
	jmp __ksaveRegs
ASM_END_FUNCTION(vector110)

ASM_GLOBAL_FUNCTION(vector111)
	pushl $0
	pushl $111
	jmp __ksaveRegs
ASM_END_FUNCTION(vector111)

ASM_GLOBAL_FUNCTION(vector112)
	pushl $0
	pushl $112
	jmp __ksaveRegs
ASM_END_FUNCTION(vector112)

ASM_GLOBAL_FUNCTION(vector113)
	pushl $0
	pushl $113
	jmp __ksaveRegs
ASM_END_FUNCTION(vector113)

ASM_GLOBAL_FUNCTION(vector114)
	pushl $0
	pushl $114
	jmp __ksaveRegs
ASM_END_FUNCTION(vector114)

ASM_GLOBAL_FUNCTION(vector115)
	pushl $0
	pushl $115
	jmp __ksaveRegs
ASM_END_FUNCTION(vector115)

ASM_GLOBAL_FUNCTION(vector116)
	pushl $0
	pushl $116
	jmp __ksaveRegs
ASM_END_FUNCTION(vector116)

ASM_GLOBAL_FUNCTION(vector117)
	pushl $0
	pushl $117
	jmp __ksaveRegs
ASM_END_FUNCTION(vector117)

ASM_GLOBAL_FUNCTION(vector118)
	pushl $0
	pushl $118
	jmp __ksaveRegs
ASM_END_FUNCTION(vector118)

ASM_GLOBAL_FUNCTION(vector119)
	pushl $0
	pushl $119
	jmp __ksaveRegs
ASM_END_FUNCTION(vector119)

ASM_GLOBAL_FUNCTION(vector120)
	pushl $0
	pushl $120
	jmp __ksaveRegs
ASM_END_FUNCTION(vector120)

ASM_GLOBAL_FUNCTION(vector121)
	pushl $0
	pushl $121
	jmp __ksaveRegs
ASM_END_FUNCTION(vector121)

ASM_GLOBAL_FUNCTION(vector122)
	pushl $0
	pushl $122
	jmp __ksaveRegs
ASM_END_FUNCTION(vector122)

ASM_GLOBAL_FUNCTION(vector123)
	pushl $0
	pushl $123
	jmp __ksaveRegs
ASM_END_FUNCTION(vector123)

ASM_GLOBAL_FUNCTION(vector124)
	pushl $0
	pushl $124
	jmp __ksaveRegs
ASM_END_FUNCTION(vector124)

ASM_GLOBAL_FUNCTION(vector125)
	pushl $0
	pushl $125
	jmp __ksaveRegs
ASM_END_FUNCTION(vector125)

ASM_GLOBAL_FUNCTION(vector126)
	pushl $0
	pushl $126
	jmp __ksaveRegs
ASM_END_FUNCTION(vector126)

ASM_GLOBAL_FUNCTION(vector127)
	pushl $0
	pushl $127
	jmp __ksaveRegs
ASM_END_FUNCTION(vector127)

ASM_GLOBAL_FUNCTION(vector128)
	pushl $0
	pushl $128
	jmp __ksaveRegs
ASM_END_FUNCTION(vector128)

ASM_GLOBAL_FUNCTION(vector129)
	pushl $0
	pushl $129
	jmp __ksaveRegs
ASM_END_FUNCTION(vector129)

ASM_GLOBAL_FUNCTION(vector130)
	pushl $0
	pushl $130
	jmp __ksaveRegs
ASM_END_FUNCTION(vector130)

ASM_GLOBAL_FUNCTION(vector131)
	pushl $0
	pushl $131
	jmp __ksaveRegs
ASM_END_FUNCTION(vector131)

ASM_GLOBAL_FUNCTION(vector132)
	pushl $0
	pushl $132
	jmp __ksaveRegs
ASM_END_FUNCTION(vector132)

ASM_GLOBAL_FUNCTION(vector133)
	pushl $0
	pushl $133
	jmp __ksaveRegs
ASM_END_FUNCTION(vector133)

ASM_GLOBAL_FUNCTION(vector134)
	pushl $0
	pushl $134
	jmp __ksaveRegs
ASM_END_FUNCTION(vector134)

ASM_GLOBAL_FUNCTION(vector135)
	pushl $0
	pushl $135
	jmp __ksaveRegs
ASM_END_FUNCTION(vector135)

ASM_GLOBAL_FUNCTION(vector136)
	pushl $0
	pushl $136
	jmp __ksaveRegs
ASM_END_FUNCTION(vector136)

ASM_GLOBAL_FUNCTION(vector137)
	pushl $0
	pushl $137
	jmp __ksaveRegs
ASM_END_FUNCTION(vector137)

ASM_GLOBAL_FUNCTION(vector138)
	pushl $0
	pushl $138
	jmp __ksaveRegs
ASM_END_FUNCTION(vector138)

ASM_GLOBAL_FUNCTION(vector139)
	pushl $0
	pushl $139
	jmp __ksaveRegs
ASM_END_FUNCTION(vector139)

ASM_GLOBAL_FUNCTION(vector140)
	pushl $0
	pushl $140
	jmp __ksaveRegs
ASM_END_FUNCTION(vector140)

ASM_GLOBAL_FUNCTION(vector141)
	pushl $0
	pushl $141
	jmp __ksaveRegs
ASM_END_FUNCTION(vector141)

ASM_GLOBAL_FUNCTION(vector142)
	pushl $0
	pushl $142
	jmp __ksaveRegs
ASM_END_FUNCTION(vector142)

ASM_GLOBAL_FUNCTION(vector143)
	pushl $0
	pushl $143
	jmp __ksaveRegs
ASM_END_FUNCTION(vector143)

ASM_GLOBAL_FUNCTION(vector144)
	pushl $0
	pushl $144
	jmp __ksaveRegs
ASM_END_FUNCTION(vector144)

ASM_GLOBAL_FUNCTION(vector145)
	pushl $0
	pushl $145
	jmp __ksaveRegs
ASM_END_FUNCTION(vector145)

ASM_GLOBAL_FUNCTION(vector146)
	pushl $0
	pushl $146
	jmp __ksaveRegs
ASM_END_FUNCTION(vector146)

ASM_GLOBAL_FUNCTION(vector147)
	pushl $0
	pushl $147
	jmp __ksaveRegs
ASM_END_FUNCTION(vector147)

ASM_GLOBAL_FUNCTION(vector148)
	pushl $0
	pushl $148
	jmp __ksaveRegs
ASM_END_FUNCTION(vector148)

ASM_GLOBAL_FUNCTION(vector149)
	pushl $0
	pushl $149
	jmp __ksaveRegs
ASM_END_FUNCTION(vector149)

ASM_GLOBAL_FUNCTION(vector150)
	pushl $0
	pushl $150
	jmp __ksaveRegs
ASM_END_FUNCTION(vector150)

ASM_GLOBAL_FUNCTION(vector151)
	pushl $0
	pushl $151
	jmp __ksaveRegs
ASM_END_FUNCTION(vector151)

ASM_GLOBAL_FUNCTION(vector152)
	pushl $0
	pushl $152
	jmp __ksaveRegs
ASM_END_FUNCTION(vector152)

ASM_GLOBAL_FUNCTION(vector153)
	pushl $0
	pushl $153
	jmp __ksaveRegs
ASM_END_FUNCTION(vector153)

ASM_GLOBAL_FUNCTION(vector154)
	pushl $0
	pushl $154
	jmp __ksaveRegs
ASM_END_FUNCTION(vector154)

ASM_GLOBAL_FUNCTION(vector155)
	pushl $0
	pushl $155
	jmp __ksaveRegs
ASM_END_FUNCTION(vector155)

ASM_GLOBAL_FUNCTION(vector156)
	pushl $0
	pushl $156
	jmp __ksaveRegs
ASM_END_FUNCTION(vector156)

ASM_GLOBAL_FUNCTION(vector157)
	pushl $0
	pushl $157
	jmp __ksaveRegs
ASM_END_FUNCTION(vector157)

ASM_GLOBAL_FUNCTION(vector158)
	pushl $0
	pushl $158
	jmp __ksaveRegs
ASM_END_FUNCTION(vector158)

ASM_GLOBAL_FUNCTION(vector159)
	pushl $0
	pushl $159
	jmp __ksaveRegs
ASM_END_FUNCTION(vector159)

ASM_GLOBAL_FUNCTION(vector160)
	pushl $0
	pushl $160
	jmp __ksaveRegs
ASM_END_FUNCTION(vector160)

ASM_GLOBAL_FUNCTION(vector161)
	pushl $0
	pushl $161
	jmp __ksaveRegs
ASM_END_FUNCTION(vector161)

ASM_GLOBAL_FUNCTION(vector162)
	pushl $0
	pushl $162
	jmp __ksaveRegs
ASM_END_FUNCTION(vector162)

ASM_GLOBAL_FUNCTION(vector163)
	pushl $0
	pushl $163
	jmp __ksaveRegs
ASM_END_FUNCTION(vector163)

ASM_GLOBAL_FUNCTION(vector164)
	pushl $0
	pushl $164
	jmp __ksaveRegs
ASM_END_FUNCTION(vector164)

ASM_GLOBAL_FUNCTION(vector165)
	pushl $0
	pushl $165
	jmp __ksaveRegs
ASM_END_FUNCTION(vector165)

ASM_GLOBAL_FUNCTION(vector166)
	pushl $0
	pushl $166
	jmp __ksaveRegs
ASM_END_FUNCTION(vector166)

ASM_GLOBAL_FUNCTION(vector167)
	pushl $0
	pushl $167
	jmp __ksaveRegs
ASM_END_FUNCTION(vector167)

ASM_GLOBAL_FUNCTION(vector168)
	pushl $0
	pushl $168
	jmp __ksaveRegs
ASM_END_FUNCTION(vector168)

ASM_GLOBAL_FUNCTION(vector169)
	pushl $0
	pushl $169
	jmp __ksaveRegs
ASM_END_FUNCTION(vector169)

ASM_GLOBAL_FUNCTION(vector170)
	pushl $0
	pushl $170
	jmp __ksaveRegs
ASM_END_FUNCTION(vector170)

ASM_GLOBAL_FUNCTION(vector171)
	pushl $0
	pushl $171
	jmp __ksaveRegs
ASM_END_FUNCTION(vector171)

ASM_GLOBAL_FUNCTION(vector172)
	pushl $0
	pushl $172
	jmp __ksaveRegs
ASM_END_FUNCTION(vector172)

ASM_GLOBAL_FUNCTION(vector173)
	pushl $0
	pushl $173
	jmp __ksaveRegs
ASM_END_FUNCTION(vector173)

ASM_GLOBAL_FUNCTION(vector174)
	pushl $0
	pushl $174
	jmp __ksaveRegs
ASM_END_FUNCTION(vector174)

ASM_GLOBAL_FUNCTION(vector175)
	pushl $0
	pushl $175
	jmp __ksaveRegs
ASM_END_FUNCTION(vector175)

ASM_GLOBAL_FUNCTION(vector176)
	pushl $0
	pushl $176
	jmp __ksaveRegs
ASM_END_FUNCTION(vector176)

ASM_GLOBAL_FUNCTION(vector177)
	pushl $0
	pushl $177
	jmp __ksaveRegs
ASM_END_FUNCTION(vector177)

ASM_GLOBAL_FUNCTION(vector178)
	pushl $0
	pushl $178
	jmp __ksaveRegs
ASM_END_FUNCTION(vector178)

ASM_GLOBAL_FUNCTION(vector179)
	pushl $0
	pushl $179
	jmp __ksaveRegs
ASM_END_FUNCTION(vector179)

ASM_GLOBAL_FUNCTION(vector180)
	pushl $0
	pushl $180
	jmp __ksaveRegs
ASM_END_FUNCTION(vector180)

ASM_GLOBAL_FUNCTION(vector181)
	pushl $0
	pushl $181
	jmp __ksaveRegs
ASM_END_FUNCTION(vector181)

ASM_GLOBAL_FUNCTION(vector182)
	pushl $0
	pushl $182
	jmp __ksaveRegs
ASM_END_FUNCTION(vector182)

ASM_GLOBAL_FUNCTION(vector183)
	pushl $0
	pushl $183
	jmp __ksaveRegs
ASM_END_FUNCTION(vector183)

ASM_GLOBAL_FUNCTION(vector184)
	pushl $0
	pushl $184
	jmp __ksaveRegs
ASM_END_FUNCTION(vector184)

ASM_GLOBAL_FUNCTION(vector185)
	pushl $0
	pushl $185
	jmp __ksaveRegs
ASM_END_FUNCTION(vector185)

ASM_GLOBAL_FUNCTION(vector186)
	pushl $0
	pushl $186
	jmp __ksaveRegs
ASM_END_FUNCTION(vector186)

ASM_GLOBAL_FUNCTION(vector187)
	pushl $0
	pushl $187
	jmp __ksaveRegs
ASM_END_FUNCTION(vector187)

ASM_GLOBAL_FUNCTION(vector188)
	pushl $0
	pushl $188
	jmp __ksaveRegs
ASM_END_FUNCTION(vector188)

ASM_GLOBAL_FUNCTION(vector189)
	pushl $0
	pushl $189
	jmp __ksaveRegs
ASM_END_FUNCTION(vector189)

ASM_GLOBAL_FUNCTION(vector190)
	pushl $0
	pushl $190
	jmp __ksaveRegs
ASM_END_FUNCTION(vector190)

ASM_GLOBAL_FUNCTION(vector191)
	pushl $0
	pushl $191
	jmp __ksaveRegs
ASM_END_FUNCTION(vector191)

ASM_GLOBAL_FUNCTION(vector192)
	pushl $0
	pushl $192
	jmp __ksaveRegs
ASM_END_FUNCTION(vector192)

ASM_GLOBAL_FUNCTION(vector193)
	pushl $0
	pushl $193
	jmp __ksaveRegs
ASM_END_FUNCTION(vector193)

ASM_GLOBAL_FUNCTION(vector194)
	pushl $0
	pushl $194
	jmp __ksaveRegs
ASM_END_FUNCTION(vector194)

ASM_GLOBAL_FUNCTION(vector195)
	pushl $0
	pushl $195
	jmp __ksaveRegs
ASM_END_FUNCTION(vector195)

ASM_GLOBAL_FUNCTION(vector196)
	pushl $0
	pushl $196
	jmp __ksaveRegs
ASM_END_FUNCTION(vector196)

ASM_GLOBAL_FUNCTION(vector197)
	pushl $0
	pushl $197
	jmp __ksaveRegs
ASM_END_FUNCTION(vector197)

ASM_GLOBAL_FUNCTION(vector198)
	pushl $0
	pushl $198
	jmp __ksaveRegs
ASM_END_FUNCTION(vector198)

ASM_GLOBAL_FUNCTION(vector199)
	pushl $0
	pushl $199
	jmp __ksaveRegs
ASM_END_FUNCTION(vector199)

ASM_GLOBAL_FUNCTION(vector200)
	pushl $0
	pushl $200
	jmp __ksaveRegs
ASM_END_FUNCTION(vector200)

ASM_GLOBAL_FUNCTION(vector201)
	pushl $0
	pushl $201
	jmp __ksaveRegs
ASM_END_FUNCTION(vector201)

ASM_GLOBAL_FUNCTION(vector202)
	pushl $0
	pushl $202
	jmp __ksaveRegs
ASM_END_FUNCTION(vector202)

ASM_GLOBAL_FUNCTION(vector203)
	pushl $0
	pushl $203
	jmp __ksaveRegs
ASM_END_FUNCTION(vector203)

ASM_GLOBAL_FUNCTION(vector204)
	pushl $0
	pushl $204
	jmp __ksaveRegs
ASM_END_FUNCTION(vector204)

ASM_GLOBAL_FUNCTION(vector205)
	pushl $0
	pushl $205
	jmp __ksaveRegs
ASM_END_FUNCTION(vector205)

ASM_GLOBAL_FUNCTION(vector206)
	pushl $0
	pushl $206
	jmp __ksaveRegs
ASM_END_FUNCTION(vector206)

ASM_GLOBAL_FUNCTION(vector207)
	pushl $0
	pushl $207
	jmp __ksaveRegs
ASM_END_FUNCTION(vector207)

ASM_GLOBAL_FUNCTION(vector208)
	pushl $0
	pushl $208
	jmp __ksaveRegs
ASM_END_FUNCTION(vector208)

ASM_GLOBAL_FUNCTION(vector209)
	pushl $0
	pushl $209
	jmp __ksaveRegs
ASM_END_FUNCTION(vector209)

ASM_GLOBAL_FUNCTION(vector210)
	pushl $0
	pushl $210
	jmp __ksaveRegs
ASM_END_FUNCTION(vector210)

ASM_GLOBAL_FUNCTION(vector211)
	pushl $0
	pushl $211
	jmp __ksaveRegs
ASM_END_FUNCTION(vector211)

ASM_GLOBAL_FUNCTION(vector212)
	pushl $0
	pushl $212
	jmp __ksaveRegs
ASM_END_FUNCTION(vector212)

ASM_GLOBAL_FUNCTION(vector213)
	pushl $0
	pushl $213
	jmp __ksaveRegs
ASM_END_FUNCTION(vector213)

ASM_GLOBAL_FUNCTION(vector214)
	pushl $0
	pushl $214
	jmp __ksaveRegs
ASM_END_FUNCTION(vector214)

ASM_GLOBAL_FUNCTION(vector215)
	pushl $0
	pushl $215
	jmp __ksaveRegs
ASM_END_FUNCTION(vector215)

ASM_GLOBAL_FUNCTION(vector216)
	pushl $0
	pushl $216
	jmp __ksaveRegs
ASM_END_FUNCTION(vector216)

ASM_GLOBAL_FUNCTION(vector217)
	pushl $0
	pushl $217
	jmp __ksaveRegs
ASM_END_FUNCTION(vector217)

ASM_GLOBAL_FUNCTION(vector218)
	pushl $0
	pushl $218
	jmp __ksaveRegs
ASM_END_FUNCTION(vector218)

ASM_GLOBAL_FUNCTION(vector219)
	pushl $0
	pushl $219
	jmp __ksaveRegs
ASM_END_FUNCTION(vector219)

ASM_GLOBAL_FUNCTION(vector220)
	pushl $0
	pushl $220
	jmp __ksaveRegs
ASM_END_FUNCTION(vector220)

ASM_GLOBAL_FUNCTION(vector221)
	pushl $0
	pushl $221
	jmp __ksaveRegs
ASM_END_FUNCTION(vector221)

ASM_GLOBAL_FUNCTION(vector222)
	pushl $0
	pushl $222
	jmp __ksaveRegs
ASM_END_FUNCTION(vector222)

ASM_GLOBAL_FUNCTION(vector223)
	pushl $0
	pushl $223
	jmp __ksaveRegs
ASM_END_FUNCTION(vector223)

ASM_GLOBAL_FUNCTION(vector224)
	pushl $0
	pushl $224
	jmp __ksaveRegs
ASM_END_FUNCTION(vector224)

ASM_GLOBAL_FUNCTION(vector225)
	pushl $0
	pushl $225
	jmp __ksaveRegs
ASM_END_FUNCTION(vector225)

ASM_GLOBAL_FUNCTION(vector226)
	pushl $0
	pushl $226
	jmp __ksaveRegs
ASM_END_FUNCTION(vector226)

ASM_GLOBAL_FUNCTION(vector227)
	pushl $0
	pushl $227
	jmp __ksaveRegs
ASM_END_FUNCTION(vector227)

ASM_GLOBAL_FUNCTION(vector228)
	pushl $0
	pushl $228
	jmp __ksaveRegs
ASM_END_FUNCTION(vector228)

ASM_GLOBAL_FUNCTION(vector229)
	pushl $0
	pushl $229
	jmp __ksaveRegs
ASM_END_FUNCTION(vector229)

ASM_GLOBAL_FUNCTION(vector230)
	pushl $0
	pushl $230
	jmp __ksaveRegs
ASM_END_FUNCTION(vector230)

ASM_GLOBAL_FUNCTION(vector231)
	pushl $0
	pushl $231
	jmp __ksaveRegs
ASM_END_FUNCTION(vector231)

ASM_GLOBAL_FUNCTION(vector232)
	pushl $0
	pushl $232
	jmp __ksaveRegs
ASM_END_FUNCTION(vector232)

ASM_GLOBAL_FUNCTION(vector233)
	pushl $0
	pushl $233
	jmp __ksaveRegs
ASM_END_FUNCTION(vector233)

ASM_GLOBAL_FUNCTION(vector234)
	pushl $0
	pushl $234
	jmp __ksaveRegs
ASM_END_FUNCTION(vector234)

ASM_GLOBAL_FUNCTION(vector235)
	pushl $0
	pushl $235
	jmp __ksaveRegs
ASM_END_FUNCTION(vector235)

ASM_GLOBAL_FUNCTION(vector236)
	pushl $0
	pushl $236
	jmp __ksaveRegs
ASM_END_FUNCTION(vector236)

ASM_GLOBAL_FUNCTION(vector237)
	pushl $0
	pushl $237
	jmp __ksaveRegs
ASM_END_FUNCTION(vector237)

ASM_GLOBAL_FUNCTION(vector238)
	pushl $0
	pushl $238
	jmp __ksaveRegs
ASM_END_FUNCTION(vector238)

ASM_GLOBAL_FUNCTION(vector239)
	pushl $0
	pushl $239
	jmp __ksaveRegs
ASM_END_FUNCTION(vector239)

ASM_GLOBAL_FUNCTION(vector240)
	pushl $0
	pushl $240
	jmp __ksaveRegs
ASM_END_FUNCTION(vector240)

ASM_GLOBAL_FUNCTION(vector241)
	pushl $0
	pushl $241
	jmp __ksaveRegs
ASM_END_FUNCTION(vector241)

ASM_GLOBAL_FUNCTION(vector242)
	pushl $0
	pushl $242
	jmp __ksaveRegs
ASM_END_FUNCTION(vector242)

ASM_GLOBAL_FUNCTION(vector243)
	pushl $0
	pushl $243
	jmp __ksaveRegs
ASM_END_FUNCTION(vector243)

ASM_GLOBAL_FUNCTION(vector244)
	pushl $0
	pushl $244
	jmp __ksaveRegs
ASM_END_FUNCTION(vector244)

ASM_GLOBAL_FUNCTION(vector245)
	pushl $0
	pushl $245
	jmp __ksaveRegs
ASM_END_FUNCTION(vector245)

ASM_GLOBAL_FUNCTION(vector246)
	pushl $0
	pushl $246
	jmp __ksaveRegs
ASM_END_FUNCTION(vector246)

ASM_GLOBAL_FUNCTION(vector247)
	pushl $0
	pushl $247
	jmp __ksaveRegs
ASM_END_FUNCTION(vector247)

ASM_GLOBAL_FUNCTION(vector248)
	pushl $0
	pushl $248
	jmp __ksaveRegs
ASM_END_FUNCTION(vector248)

ASM_GLOBAL_FUNCTION(vector249)
	pushl $0
	pushl $249
	jmp __ksaveRegs
ASM_END_FUNCTION(vector249)

ASM_GLOBAL_FUNCTION(vector250)
	pushl $0
	pushl $250
	jmp __ksaveRegs
ASM_END_FUNCTION(vector250)

ASM_GLOBAL_FUNCTION(vector251)
	pushl $0
	pushl $251
	jmp __ksaveRegs
ASM_END_FUNCTION(vector251)

ASM_GLOBAL_FUNCTION(vector252)
	pushl $0
	pushl $252
	jmp __ksaveRegs
ASM_END_FUNCTION(vector252)

ASM_GLOBAL_FUNCTION(vector253)
	pushl $0
	pushl $253
	jmp __ksaveRegs
ASM_END_FUNCTION(vector253)

ASM_GLOBAL_FUNCTION(vector254)
	pushl $0
	pushl $254
	jmp __ksaveRegs
ASM_END_FUNCTION(vector254)

ASM_GLOBAL_FUNCTION(vector255)
	pushl $0
	pushl $255
	jmp __ksaveRegs
ASM_END_FUNCTION(vector255)
	
