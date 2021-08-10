#include "string.h"
#include <stddef.h>
#include <stdint.h>

typedef unsigned char byte;
#define op_t unsigned long int
#define OPSIZ (sizeof(op_t))

void *memset(void *dstpp, char c, size_t len)
{
    long int dstp = (long int)dstpp;
    if (len >= 8)
	{
		size_t xlen;
		op_t cccc;

		cccc = (unsigned char)c;
		cccc |= cccc << 8;
		cccc |= cccc << 16;
		if (OPSIZ > 4)
			/* Do the shift in two steps to avoid warning if long has 32 bits.  */
			cccc |= (cccc << 16) << 16;

		/* There are at least some bytes to set.
	 No need to test for LEN == 0 in this alignment loop.  */
		while (dstp % OPSIZ != 0)
		{
			((byte *)dstp)[0] = c;
			dstp += 1;
			len -= 1;
		}

		/* Write 8 `op_t' per iteration until less than 8 `op_t' remain.  */
		xlen = len / (OPSIZ * 8);
		while (xlen > 0)
		{
			((op_t *)dstp)[0] = cccc;
			((op_t *)dstp)[1] = cccc;
			((op_t *)dstp)[2] = cccc;
			((op_t *)dstp)[3] = cccc;
			((op_t *)dstp)[4] = cccc;
			((op_t *)dstp)[5] = cccc;
			((op_t *)dstp)[6] = cccc;
			((op_t *)dstp)[7] = cccc;
			dstp += 8 * OPSIZ;
			xlen -= 1;
		}
		len %= OPSIZ * 8;

		/* Write 1 `op_t' per iteration until less than OPSIZ bytes remain.  */
		xlen = len / OPSIZ;
		while (xlen > 0)
		{
			((op_t *)dstp)[0] = cccc;
			dstp += OPSIZ;
			xlen -= 1;
		}
		len %= OPSIZ;
	}

	/* Write the last few bytes.  */
	while (len > 0)
	{
		((byte *)dstp)[0] = c;
		dstp += 1;
		len -= 1;
	}

	return dstpp;
}

#define OP_T_THRES 8

#define BYTE_COPY_FWD(dst_bp, src_bp, nbytes)                                          \
	do                                                                                 \
	{                                                                                  \
		int __d0;                                                                      \
		asm volatile(		 /* Clear the direction flag, so copying goes forward.  */ \
					 "cld\n" /* Copy bytes.  */                                        \
					 "rep\n"                                                           \
					 "movsb"                                                           \
					 : "=D"(dst_bp), "=S"(src_bp), "=c"(__d0)                          \
					 : "0"(dst_bp), "1"(src_bp), "2"(nbytes)                           \
					 : "memory");                                                      \
	} while (0)

#define BYTE_COPY_BWD(dst_ep, src_ep, nbytes)                                             \
	do                                                                                    \
	{                                                                                     \
		int __d0;                                                                         \
		asm volatile(		 /* Set the direction flag, so copying goes backwards.  */    \
					 "std\n" /* Copy bytes.  */                                           \
					 "rep\n"                                                              \
					 "movsb\n" /* Clear the dir flag.  Convention says it should be 0. */ \
					 "cld"                                                                \
					 : "=D"(dst_ep), "=S"(src_ep), "=c"(__d0)                             \
					 : "0"(dst_ep - 1), "1"(src_ep - 1), "2"(nbytes)                      \
					 : "memory");                                                         \
		dst_ep += 1;                                                                      \
		src_ep += 1;                                                                      \
	} while (0)

#define WORD_COPY_FWD(dst_bp, src_bp, nbytes_left, nbytes)                             \
	do                                                                                 \
	{                                                                                  \
		int __d0;                                                                      \
		asm volatile(		 /* Clear the direction flag, so copying goes forward.  */ \
					 "cld\n" /* Copy longwords.  */                                    \
					 "rep\n"                                                           \
					 "movsl"                                                           \
					 : "=D"(dst_bp), "=S"(src_bp), "=c"(__d0)                          \
					 : "0"(dst_bp), "1"(src_bp), "2"((nbytes) / 4)                     \
					 : "memory");                                                      \
		(nbytes_left) = (nbytes) % 4;                                                  \
	} while (0)

#define WORD_COPY_BWD(dst_ep, src_ep, nbytes_left, nbytes)                                \
	do                                                                                    \
	{                                                                                     \
		int __d0;                                                                         \
		asm volatile(		 /* Set the direction flag, so copying goes backwards.  */    \
					 "std\n" /* Copy longwords.  */                                       \
					 "rep\n"                                                              \
					 "movsl\n" /* Clear the dir flag.  Convention says it should be 0. */ \
					 "cld"                                                                \
					 : "=D"(dst_ep), "=S"(src_ep), "=c"(__d0)                             \
					 : "0"(dst_ep - 4), "1"(src_ep - 4), "2"((nbytes) / 4)                \
					 : "memory");                                                         \
		dst_ep += 4;                                                                      \
		src_ep += 4;                                                                      \
		(nbytes_left) = (nbytes) % 4;                                                     \
	} while (0)

void *memcpy(void *dstpp, const void *srcpp, size_t len)
{
	unsigned long int dstp = (long int)dstpp;
	unsigned long int srcp = (long int)srcpp;

	/* Copy from the beginning to the end.  */

	/* If there not too few bytes to copy, use word copy.  */
	if (len >= OP_T_THRES)
	{
		/* Copy just a few bytes to make DSTP aligned.  */
		len -= (-dstp) % OPSIZ;
		BYTE_COPY_FWD(dstp, srcp, (-dstp) % OPSIZ);

		/* Copy from SRCP to DSTP taking advantage of the known alignment of
	 DSTP.  Number of bytes remaining is put in the third argument,
	 i.e. in LEN.  This number may vary from machine to machine.  */

		WORD_COPY_FWD(dstp, srcp, len, len);

		/* Fall out and copy the tail.  */
	}

	/* There are just a few bytes to copy.  Use byte memory operations.  */
	BYTE_COPY_FWD(dstp, srcp, len);

	return dstpp;
}