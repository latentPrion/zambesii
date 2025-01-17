
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

ASM_GLOBAL_DATA(__kvectorTable)
	.long	vector0,	vector1,	vector2,	vector3
	.long	vector4,	vector5,	vector6,	vector7
	.long	vector8,	vector9,	vector10,	vector11
	.long	vector12,	vector13,	vector14,	vector15
	.long	vector16,	vector17,	vector18,	vector19
	.long	vector20,	vector21,	vector22,	vector23
	.long	vector24,	vector25,	vector26,	vector27
	.long	vector28,	vector29,	vector30,	vector31
	.long	vector32,	vector33,	vector34,	vector35
	.long	vector36,	vector37,	vector38,	vector39
	.long	vector40,	vector41,	vector42,	vector43
	.long	vector44,	vector45,	vector46,	vector47
	.long	vector48,	vector49,	vector50,	vector51
	.long	vector52,	vector53,	vector54,	vector55
	.long	vector56,	vector57,	vector58,	vector59
	.long	vector60,	vector61,	vector62,	vector63
	.long	vector64,	vector65,	vector66,	vector67
	.long	vector68,	vector69,	vector70,	vector71
	.long	vector72,	vector73,	vector74,	vector75
	.long	vector76,	vector77,	vector78,	vector79
	.long	vector80,	vector81,	vector82,	vector83
	.long	vector84,	vector85,	vector86,	vector87
	.long	vector88,	vector89,	vector90,	vector91
	.long	vector92,	vector93,	vector94,	vector95
	.long	vector96,	vector97,	vector98,	vector99
	.long	vector100,	vector101,	vector102,	vector103
	.long	vector104,	vector105,	vector106,	vector107
	.long	vector108,	vector109,	vector110,	vector111
	.long	vector112,	vector113,	vector114,	vector115
	.long	vector116,	vector117,	vector118,	vector119
	.long	vector120,	vector121,	vector122,	vector123
	.long	vector124,	vector125,	vector126,	vector127
	.long	vector128,	vector129,	vector130,	vector131
	.long	vector132,	vector133,	vector134,	vector135
	.long	vector136,	vector137,	vector138,	vector139
	.long	vector140,	vector141,	vector142,	vector143
	.long	vector144,	vector145,	vector146,	vector147
	.long	vector148,	vector149,	vector150,	vector151
	.long	vector152,	vector153,	vector154,	vector155
	.long	vector156,	vector157,	vector158,	vector159
	.long	vector160,	vector161,	vector162,	vector163
	.long	vector164,	vector165,	vector166,	vector167
	.long	vector168,	vector169,	vector170,	vector171
	.long	vector172,	vector173,	vector174,	vector175
	.long	vector176,	vector177,	vector178,	vector179
	.long	vector180,	vector181,	vector182,	vector183
	.long	vector184,	vector185,	vector186,	vector187
	.long	vector188,	vector189,	vector190,	vector191
	.long	vector192,	vector193,	vector194,	vector195
	.long	vector196,	vector197,	vector198,	vector199
	.long	vector200,	vector201,	vector202,	vector203
	.long	vector204,	vector205,	vector206,	vector207
	.long	vector208,	vector209,	vector210,	vector211
	.long	vector212,	vector213,	vector214,	vector215
	.long	vector216,	vector217,	vector218,	vector219
	.long	vector220,	vector221,	vector222,	vector223
	.long	vector224,	vector225,	vector226,	vector227
	.long	vector228,	vector229,	vector230,	vector231
	.long	vector232,	vector233,	vector234,	vector235
	.long	vector236,	vector237,	vector238,	vector239
	.long	vector240,	vector241,	vector242,	vector243
	.long	vector244,	vector245,	vector246,	vector247
	.long	vector248,	vector249,	vector250,	vector251
	.long	vector252,	vector253,	vector254,	vector255
ASM_END_DATA(__kvectorTable)

ASM_LOCAL_FUNCTION(__ksaveRegs)
	pusha

	/* EAX = (%ds << $16) | %es ; EBX = (%fs << 16) | %gs */
	movw	%fs, %bx
	shl	$16, %ebx
	movw	%gs, %bx
	movw	%ds, %ax
	shl	$16, %eax
	movw	%es, %ax

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

	call	interruptTrib_interruptEntry

	/* From a technical POV, Zambesii never should reach here. When we
	 * return from a interrupt context, we simply point the stack pointer
	 * to the context structure and pop and ret, all from within inline ASM
	 * in C.
	 *
	 * Reaching here is a bad thing(TM) in that sense. But by no means
	 * should our kernel crash simply because of that. We should then just
	 * do the normal thing and then IRET.
	 **/
ASM_END_FUNCTION(__ksaveRegs)

ASM_GLOBAL_FUNCTION(saveContextAndCallPull)
	/**	EXPLANATION:
	 * We were called here using the ABI;
	 * 	ESP+0 = return address.
	 *
	 * Push all state to the stack in the same order that the CPU does
	 * naturally on IRQ entry, minus SS + ESP.
	 **/
	pushl	%eax
	movl	0x4(%esp), %eax
	pushl	%eax
	pushf
	movl	(%esp), %eax
	movl	%eax, 12(%esp)
	movl	8(%esp), %eax
	movl	%eax, (%esp)
	xor	%eax, %eax
	movw	%cs, %ax
	movl	%eax, 8(%esp)
	popl	%eax

	subl	$8, %esp

	pushal

	pushw	%fs
	pushw	%gs
	pushw	%ds
	pushw	%es

	/* Save the end address of the register context we just pushed */
	movl	%esp, %eax

	/* Get the CPU's sleep stack base addr (ESP+64). */
	movl	60(%esp), %esp

	/* We are now operating on the CPU's sleep stack. cpuStream_pull()
	 * takes a single argument, which is the address of the beginning of the
	 * register context for the task whose context we just saved.
	 *
	 * This argument is used to set the correct context address in the
	 * task's thread control block, so that when the task is being resumed,
	 * we know where to pop its context from.
	 *
	 * If NULL is passed as the argument, it means that the task's context
	 * was not saved. That is, the task was just terminated.
	 **/
	pushl	%eax
	call	taskStream_pull
ASM_END_FUNCTION(saveContextAndCallPull)

ASM_GLOBAL_FUNCTION(loadContextAndJump)
	/**	EXPLANATION:
	 * void loadContextAndJump(RegisterContext *context);
	 **/
	/* At this point, ESP+0=retAddr, ESP+4=context. */
	movl	0x4(%esp), %esp
	popw	%es
	popw	%ds
	popw	%gs
	popw	%fs

	popal

	addl	$8, %esp
	iretl
ASM_END_FUNCTION(loadContextAndJump)

/**	EXPLANATION:
 * Vector handlers: they push the vector number, and, depending on the vector,
 * a value to even off the stack.
 **/

.macro DEFINE_VECTOR_NOERR number
ASM_LOCAL_FUNCTION(vector\()\number\())
    pushl $0
    pushl $\number
    jmp __ksaveRegs
ASM_END_FUNCTION(vector\()\number\())
.endm

.macro DEFINE_VECTOR_ERR number
ASM_LOCAL_FUNCTION(vector\()\number\())
    pushl $\number
    jmp __ksaveRegs
ASM_END_FUNCTION(vector\()\number\())
.endm

DEFINE_VECTOR_NOERR 0
DEFINE_VECTOR_NOERR 1
DEFINE_VECTOR_NOERR 2
DEFINE_VECTOR_NOERR 3
DEFINE_VECTOR_NOERR 4
DEFINE_VECTOR_NOERR 5
DEFINE_VECTOR_NOERR 6
DEFINE_VECTOR_NOERR 7
DEFINE_VECTOR_ERR   8
DEFINE_VECTOR_NOERR 9
DEFINE_VECTOR_ERR   10
DEFINE_VECTOR_ERR   11
DEFINE_VECTOR_ERR   12
DEFINE_VECTOR_ERR   13
DEFINE_VECTOR_ERR   14
DEFINE_VECTOR_NOERR 15
DEFINE_VECTOR_NOERR 16
DEFINE_VECTOR_NOERR 17
DEFINE_VECTOR_NOERR 18
DEFINE_VECTOR_NOERR 19
DEFINE_VECTOR_NOERR 20
DEFINE_VECTOR_NOERR 21
DEFINE_VECTOR_NOERR 22
DEFINE_VECTOR_NOERR 23
DEFINE_VECTOR_NOERR 24
DEFINE_VECTOR_NOERR 25
DEFINE_VECTOR_NOERR 26
DEFINE_VECTOR_NOERR 27
DEFINE_VECTOR_NOERR 28
DEFINE_VECTOR_NOERR 29
DEFINE_VECTOR_NOERR 30
DEFINE_VECTOR_NOERR 31
DEFINE_VECTOR_NOERR 32
DEFINE_VECTOR_NOERR 33
DEFINE_VECTOR_NOERR 34
DEFINE_VECTOR_NOERR 35
DEFINE_VECTOR_NOERR 36
DEFINE_VECTOR_NOERR 37
DEFINE_VECTOR_NOERR 38
DEFINE_VECTOR_NOERR 39
DEFINE_VECTOR_NOERR 40
DEFINE_VECTOR_NOERR 41
DEFINE_VECTOR_NOERR 42
DEFINE_VECTOR_NOERR 43
DEFINE_VECTOR_NOERR 44
DEFINE_VECTOR_NOERR 45
DEFINE_VECTOR_NOERR 46
DEFINE_VECTOR_NOERR 47
DEFINE_VECTOR_NOERR 48
DEFINE_VECTOR_NOERR 49
DEFINE_VECTOR_NOERR 50
DEFINE_VECTOR_NOERR 51
DEFINE_VECTOR_NOERR 52
DEFINE_VECTOR_NOERR 53
DEFINE_VECTOR_NOERR 54
DEFINE_VECTOR_NOERR 55
DEFINE_VECTOR_NOERR 56
DEFINE_VECTOR_NOERR 57
DEFINE_VECTOR_NOERR 58
DEFINE_VECTOR_NOERR 59
DEFINE_VECTOR_NOERR 60
DEFINE_VECTOR_NOERR 61
DEFINE_VECTOR_NOERR 62
DEFINE_VECTOR_NOERR 63
DEFINE_VECTOR_NOERR 64
DEFINE_VECTOR_NOERR 65
DEFINE_VECTOR_NOERR 66
DEFINE_VECTOR_NOERR 67
DEFINE_VECTOR_NOERR 68
DEFINE_VECTOR_NOERR 69
DEFINE_VECTOR_NOERR 70
DEFINE_VECTOR_NOERR 71
DEFINE_VECTOR_NOERR 72
DEFINE_VECTOR_NOERR 73
DEFINE_VECTOR_NOERR 74
DEFINE_VECTOR_NOERR 75
DEFINE_VECTOR_NOERR 76
DEFINE_VECTOR_NOERR 77
DEFINE_VECTOR_NOERR 78
DEFINE_VECTOR_NOERR 79
DEFINE_VECTOR_NOERR 80
DEFINE_VECTOR_NOERR 81
DEFINE_VECTOR_NOERR 82
DEFINE_VECTOR_NOERR 83
DEFINE_VECTOR_NOERR 84
DEFINE_VECTOR_NOERR 85
DEFINE_VECTOR_NOERR 86
DEFINE_VECTOR_NOERR 87
DEFINE_VECTOR_NOERR 88
DEFINE_VECTOR_NOERR 89
DEFINE_VECTOR_NOERR 90
DEFINE_VECTOR_NOERR 91
DEFINE_VECTOR_NOERR 92
DEFINE_VECTOR_NOERR 93
DEFINE_VECTOR_NOERR 94
DEFINE_VECTOR_NOERR 95
DEFINE_VECTOR_NOERR 96
DEFINE_VECTOR_NOERR 97
DEFINE_VECTOR_NOERR 98
DEFINE_VECTOR_NOERR 99
DEFINE_VECTOR_NOERR 100
DEFINE_VECTOR_NOERR 101
DEFINE_VECTOR_NOERR 102
DEFINE_VECTOR_NOERR 103
DEFINE_VECTOR_NOERR 104
DEFINE_VECTOR_NOERR 105
DEFINE_VECTOR_NOERR 106
DEFINE_VECTOR_NOERR 107
DEFINE_VECTOR_NOERR 108
DEFINE_VECTOR_NOERR 109
DEFINE_VECTOR_NOERR 110
DEFINE_VECTOR_NOERR 111
DEFINE_VECTOR_NOERR 112
DEFINE_VECTOR_NOERR 113
DEFINE_VECTOR_NOERR 114
DEFINE_VECTOR_NOERR 115
DEFINE_VECTOR_NOERR 116
DEFINE_VECTOR_NOERR 117
DEFINE_VECTOR_NOERR 118
DEFINE_VECTOR_NOERR 119
DEFINE_VECTOR_NOERR 120
DEFINE_VECTOR_NOERR 121
DEFINE_VECTOR_NOERR 122
DEFINE_VECTOR_NOERR 123
DEFINE_VECTOR_NOERR 124
DEFINE_VECTOR_NOERR 125
DEFINE_VECTOR_NOERR 126
DEFINE_VECTOR_NOERR 127
DEFINE_VECTOR_NOERR 128
DEFINE_VECTOR_NOERR 129
DEFINE_VECTOR_NOERR 130
DEFINE_VECTOR_NOERR 131
DEFINE_VECTOR_NOERR 132
DEFINE_VECTOR_NOERR 133
DEFINE_VECTOR_NOERR 134
DEFINE_VECTOR_NOERR 135
DEFINE_VECTOR_NOERR 136
DEFINE_VECTOR_NOERR 137
DEFINE_VECTOR_NOERR 138
DEFINE_VECTOR_NOERR 139
DEFINE_VECTOR_NOERR 140
DEFINE_VECTOR_NOERR 141
DEFINE_VECTOR_NOERR 142
DEFINE_VECTOR_NOERR 143
DEFINE_VECTOR_NOERR 144
DEFINE_VECTOR_NOERR 145
DEFINE_VECTOR_NOERR 146
DEFINE_VECTOR_NOERR 147
DEFINE_VECTOR_NOERR 148
DEFINE_VECTOR_NOERR 149
DEFINE_VECTOR_NOERR 150
DEFINE_VECTOR_NOERR 151
DEFINE_VECTOR_NOERR 152
DEFINE_VECTOR_NOERR 153
DEFINE_VECTOR_NOERR 154
DEFINE_VECTOR_NOERR 155
DEFINE_VECTOR_NOERR 156
DEFINE_VECTOR_NOERR 157
DEFINE_VECTOR_NOERR 158
DEFINE_VECTOR_NOERR 159
DEFINE_VECTOR_NOERR 160
DEFINE_VECTOR_NOERR 161
DEFINE_VECTOR_NOERR 162
DEFINE_VECTOR_NOERR 163
DEFINE_VECTOR_NOERR 164
DEFINE_VECTOR_NOERR 165
DEFINE_VECTOR_NOERR 166
DEFINE_VECTOR_NOERR 167
DEFINE_VECTOR_NOERR 168
DEFINE_VECTOR_NOERR 169
DEFINE_VECTOR_NOERR 170
DEFINE_VECTOR_NOERR 171
DEFINE_VECTOR_NOERR 172
DEFINE_VECTOR_NOERR 173
DEFINE_VECTOR_NOERR 174
DEFINE_VECTOR_NOERR 175
DEFINE_VECTOR_NOERR 176
DEFINE_VECTOR_NOERR 177
DEFINE_VECTOR_NOERR 178
DEFINE_VECTOR_NOERR 179
DEFINE_VECTOR_NOERR 180
DEFINE_VECTOR_NOERR 181
DEFINE_VECTOR_NOERR 182
DEFINE_VECTOR_NOERR 183
DEFINE_VECTOR_NOERR 184
DEFINE_VECTOR_NOERR 185
DEFINE_VECTOR_NOERR 186
DEFINE_VECTOR_NOERR 187
DEFINE_VECTOR_NOERR 188
DEFINE_VECTOR_NOERR 189
DEFINE_VECTOR_NOERR 190
DEFINE_VECTOR_NOERR 191
DEFINE_VECTOR_NOERR 192
DEFINE_VECTOR_NOERR 193
DEFINE_VECTOR_NOERR 194
DEFINE_VECTOR_NOERR 195
DEFINE_VECTOR_NOERR 196
DEFINE_VECTOR_NOERR 197
DEFINE_VECTOR_NOERR 198
DEFINE_VECTOR_NOERR 199
DEFINE_VECTOR_NOERR 200
DEFINE_VECTOR_NOERR 201
DEFINE_VECTOR_NOERR 202
DEFINE_VECTOR_NOERR 203
DEFINE_VECTOR_NOERR 204
DEFINE_VECTOR_NOERR 205
DEFINE_VECTOR_NOERR 206
DEFINE_VECTOR_NOERR 207
DEFINE_VECTOR_NOERR 208
DEFINE_VECTOR_NOERR 209
DEFINE_VECTOR_NOERR 210
DEFINE_VECTOR_NOERR 211
DEFINE_VECTOR_NOERR 212
DEFINE_VECTOR_NOERR 213
DEFINE_VECTOR_NOERR 214
DEFINE_VECTOR_NOERR 215
DEFINE_VECTOR_NOERR 216
DEFINE_VECTOR_NOERR 217
DEFINE_VECTOR_NOERR 218
DEFINE_VECTOR_NOERR 219
DEFINE_VECTOR_NOERR 220
DEFINE_VECTOR_NOERR 221
DEFINE_VECTOR_NOERR 222
DEFINE_VECTOR_NOERR 223
DEFINE_VECTOR_NOERR 224
DEFINE_VECTOR_NOERR 225
DEFINE_VECTOR_NOERR 226
DEFINE_VECTOR_NOERR 227
DEFINE_VECTOR_NOERR 228
DEFINE_VECTOR_NOERR 229
DEFINE_VECTOR_NOERR 230
DEFINE_VECTOR_NOERR 231
DEFINE_VECTOR_NOERR 232
DEFINE_VECTOR_NOERR 233
DEFINE_VECTOR_NOERR 234
DEFINE_VECTOR_NOERR 235
DEFINE_VECTOR_NOERR 236
DEFINE_VECTOR_NOERR 237
DEFINE_VECTOR_NOERR 238
DEFINE_VECTOR_NOERR 239
DEFINE_VECTOR_NOERR 240
DEFINE_VECTOR_NOERR 241
DEFINE_VECTOR_NOERR 242
DEFINE_VECTOR_NOERR 243
DEFINE_VECTOR_NOERR 244
DEFINE_VECTOR_NOERR 245
DEFINE_VECTOR_NOERR 246
DEFINE_VECTOR_NOERR 247
DEFINE_VECTOR_NOERR 248
DEFINE_VECTOR_NOERR 249
DEFINE_VECTOR_NOERR 250
DEFINE_VECTOR_NOERR 251
DEFINE_VECTOR_NOERR 252
DEFINE_VECTOR_NOERR 253
DEFINE_VECTOR_NOERR 254
DEFINE_VECTOR_NOERR 255

