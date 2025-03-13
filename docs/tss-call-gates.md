# TSS Selectors and Call Gates:

## Protection rings/priv levels:

0 - 3: 0 = highest, 3 = userspace.


## Call Gates:

Sec 6.4.5:
> Code modules in lower privilege segments can only access modules operating at higher privilege segments by means of a tightly controlled and protected interface called a gate.

Calls to higher priv segments are handled "in a similar manner as a far call" with these differences:
* The segment selector should denote a call gate descriptor and not a normal code segment descriptor.
* The processor switches to a new stack to execute the called procedure. Each privilege level has its own stack.
  * The segment selector and stack pointer for the privilege level 3 stack are stored in the SS and ESP registers,
  * The segment selectors and stack pointers for the privilege level 2, 1, and 0 stacks are stored in a system segment called the task state segment (TSS) [unsure whether they're talking about the target segment's loadable stacks or the caller's yet-to-be-saved stacks].
