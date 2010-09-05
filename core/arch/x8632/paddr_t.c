
#include <arch/paddr_t.h>

void pp_eq(paddr_t p, paddr_t q)
{
	p.low = q.low;
	p.high = q.high;
}

void pu_eq(paddr_t p, uarch_t n)
{
	p.high = 0;
	p.low = n;
}

// Basic turing stuff.
void p_add(paddr_t p, uarch_t n)
{
	if ((p.low + n) < p.low)
void p_sub(paddr_t p, uarch_t n);
void p_mul(paddr_t p, uarch_t n);
void p_div(paddr_t p, uarch_t n);

// Binary operations.
void pp_and(paddr_t p, paddr_t q);
void pp_or(paddr_t p, paddr_t q);
void pu_and(paddr_t p, uarch_t n);
void pu_or(paddr_t p, uarch_t n);
void p_shl(paddr_t p, uarch_t n);
void p_shr(paddr_t p, uarch_t n);

