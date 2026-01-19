#ifndef __LIBRARY_LKENV_H__
#define __LIBRARY_LKENV_H__

#include <Library/ArmLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PrintLib.h>
#include <Library/TimerLib.h>

#include "minstdbool.h"
#include "minstdint.h"


#define REG32(addr) ((volatile uint32_t *)(addr))
#define writel_rt(v, a) (*REG32(a) = (v))
#define readl_rt(a) (*REG32(a))
#define errorf(fmt, ...) DEBUG((EFI_D_ERROR, fmt, ##__VA_ARGS__))
#define debugf(fmt, ...) DEBUG((EFI_D_WARN, fmt, ##__VA_ARGS__))
#define puts(fmt, ...) DEBUG((EFI_D_INFO, fmt, ##__VA_ARGS__))
#define printf(fmt, ...) DEBUG((EFI_D_WARN, fmt, ##__VA_ARGS__))
#define writeb(v, a) MmioWrite8((UINTN)(a), (UINT8)(v))
#define readb(a) MmioRead8((UINTN)(a))
#define writel(v, a) MmioWrite32((UINTN)(a), (UINT32)(v))
#define readl(a) MmioRead32((UINTN)(a))
#define writeb(v, a) MmioWrite8((UINTN)(a), (UINT8)(v))
#define readb(a) MmioRead8((UINTN)(a))
#define writew(v, a) MmioWrite16((UINTN)(a), (UINT16)(v))
#define readw(a) MmioRead16((UINTN)(a))

#define ALIGN(x,a)		__ALIGN_MASK((x),(typeof(x))(a)-1)
#define __ALIGN_MASK(x,mask)	(((x)+(mask))&~(mask))

#define ROUND(a, b)		(((a) + (b) - 1) & ~((b) - 1))

#define ARCH_DMA_MINALIGN 64
#define PAD_COUNT(s, pad) (((s) - 1) / (pad) + 1)
#define PAD_SIZE(s, pad) (PAD_COUNT(s, pad) * pad)
#define ALLOC_ALIGN_BUFFER_PAD(type, name, size, align, pad)		\
	char __##name[ROUND(PAD_SIZE((size) * sizeof(type), pad), align)  \
		      + (align - 1)];					\
									\
	type *name = (type *) ALIGN((uintptr_t)__##name, align)
#define ALLOC_ALIGN_BUFFER(type, name, size, align)		\
	ALLOC_ALIGN_BUFFER_PAD(type, name, size, align, 1)
#define ALLOC_CACHE_ALIGN_BUFFER_PAD(type, name, size, pad)		\
	ALLOC_ALIGN_BUFFER_PAD(type, name, size, ARCH_DMA_MINALIGN, pad)
#define ALLOC_CACHE_ALIGN_BUFFER(type, name, size)			\
	ALLOC_ALIGN_BUFFER(type, name, size, ARCH_DMA_MINALIGN)
#define RMWREG32(addr, startbit, width, val)                                   \
  writel(                                                                      \
      (readl(addr) & ~(((1 << (width)) - 1) << (startbit))) |                  \
          ((val) << (startbit)),                                               \
      addr)
#define 	MAX_STRING_SIZE   0x1000
#define 	sprintf(buf, ...)   AsciiSPrint(buf,MAX_STRING_SIZE,__VA_ARGS__)
typedef uint32_t __u32;
typedef __u32 __be32;
#define __be32_to_cpu(x) ((__u32)(__be32)(x))
#define LOG2(x) (((x & 0xaaaaaaaa) ? 1 : 0) + ((x & 0xcccccccc) ? 2 : 0) + \
		 ((x & 0xf0f0f0f0) ? 4 : 0) + ((x & 0xff00ff00) ? 8 : 0) + \
		 ((x & 0xffff0000) ? 16 : 0))
#define CHIP_REG_OR(reg_addr, value)    (*(volatile u32*)(reg_addr) |= (u32)(value))
#define CHIP_REG_AND(reg_addr, value)   (*(volatile u32*)(reg_addr) &= (u32)(value))
#define CHIP_REG_GET(reg_addr)          (*(volatile u32*)(reg_addr))
#define CHIP_REG_SET(reg_addr, value)   (*(volatile u32*)(reg_addr)  = (u32)(value))

#define BIT(bit) (1 << (bit))
uint32_t __div64_32(uint64_t *n, uint32_t base);
# define do_div(n,base) ({				\
	uint32_t __base = (base);			\
	uint32_t __rem;					\
	(void)(((typeof((n)) *)0) == ((uint64_t *)0));	\
	if (((n) >> 32) == 0) {			\
		__rem = (uint32_t)(n) % __base;		\
		(n) = (uint32_t)(n) / __base;		\
	} else						\
		__rem = __div64_32(&(n), __base);	\
	__rem;						\
 })

/* Wrapper for do_div(). Doesn't modify dividend and returns
 * the result, not reminder.
 */
static inline uint64_t lldiv(uint64_t dividend, uint32_t divisor)
{
	uint64_t __res = dividend;
	do_div(__res, divisor);
	return(__res);
}



#ifdef MSM_SECURE_IO
#define readl_relaxed secure_readl
#define writel_relaxed secure_writel
#else
#define readl_relaxed readl
#define writel_relaxed writel
#endif

#define NO_ERROR 0
#define ERROR -1
#define ERR_NOT_FOUND -2
#define ERR_NO_MEMORY -5
#define ERR_NOT_VALID -7
#define ERR_INVALID_ARGS -8
#define ERR_IO -20
#define ERR_NOT_SUPPORTED -24

#include "minstdbool.h"
#include "minstring.h"

#define va_list VA_LIST
#define offsetof(type, member) OFFSET_OF(type, member)
#define __PACKED __attribute__((packed))

#define ROUNDUP(a, b) (((a) + ((b)-1)) & ~((b)-1))
#define ROUNDDOWN(a, b) ((a) & ~((b)-1))
#define CACHE_LINE (ArmDataCacheLineLength())
#define IS_CACHE_LINE_ALIGNED(addr) !((UINTN)(addr) & (CACHE_LINE - 1))

#define snprintf(s, n, fmt, ...)                                               \
  ((int)AsciiSPrint((s), (n), (fmt), ##__VA_ARGS__))

/* debug levels */
#define CRITICAL DEBUG_ERROR
#define ALWAYS DEBUG_ERROR
#define INFO DEBUG_INFO
#define SPEW DEBUG_VERBOSE

#if !defined(MDEPKG_NDEBUG)
#define dprintf(level, fmt, ...)                                               \
  do {                                                                         \
    if (DebugPrintEnabled()) {                                                 \
      CHAR8 __printbuf[100];                                                   \
      UINTN __printindex;                                                      \
      CONST CHAR8 *__fmtptr = (fmt);                                           \
      UINTN        __fmtlen = AsciiStrSize(__fmtptr);                          \
      CopyMem(__printbuf, __fmtptr, __fmtlen);                                 \
      __printbuf[__fmtlen - 1] = 0;                                            \
      for (__printindex = 1; __printbuf[__printindex]; __printindex++) {       \
        if (__printbuf[__printindex - 1] == '%' &&                             \
            __printbuf[__printindex] == 's')                                   \
          __printbuf[__printindex] = 'a';                                      \
      }                                                                        \
      DEBUG(((level), __printbuf, ##__VA_ARGS__));                             \
    }                                                                          \
  } while (0)
#else
#define dprintf(level, fmt, ...)
#endif

#define ntohl(n) SwapBytes32(n)

#define dmb() ArmDataMemoryBarrier()
#define dsb() ArmDataSynchronizationBarrier()

#define mdelay(msecs) MicroSecondDelay((msecs)*1000)
#define udelay(usecs) MicroSecondDelay((usecs))

#define arch_clean_invalidate_cache_range(start, len)                          \
  WriteBackInvalidateDataCacheRange((VOID *)(UINTN)(start), (UINTN)(len))
#define arch_invalidate_cache_range(start, len)                                \
  InvalidateDataCacheRange((VOID *)(UINTN)(start), (UINTN)(len));
#define flush_cache(start_addr, trans_bytes)                         \
  do {                                                               \
    UINTN _addr = (UINTN)(start_addr);                               \
    UINTN _size = (UINTN)(trans_bytes);                              \
    if (_size > 0) {                                                 \
      arch_clean_invalidate_cache_range(_addr, _size);               \
      dmb(); \
      dsb();          \
    }                                                                \
  } while (0)
#define __ALWAYS_INLINE __attribute__((always_inline))

#define ROUND_TO_PAGE(x) (x & (~(EFI_PAGE_SIZE - 1)))

extern int errno;

#define EPERM 1     /* Not super-user */
#define ENOENT 2    /* No such file or directory */
#define ESRCH 3     /* No such process */
#define EINTR 4     /* Interrupted system call */
#define EIO 5       /* I/O error */
#define ENXIO 6     /* No such device or address */
#define E2BIG 7     /* Arg list too long */
#define ENOEXEC 8   /* Exec format error */
#define EBADF 9     /* Bad file number */
#define ECHILD 10   /* No children */
#define EAGAIN 11   /* No more processes */
#define ENOMEM 12   /* Not enough core */
#define EACCES 13   /* Permission denied */
#define EFAULT 14   /* Bad address */
#define ENOTBLK 15  /* Block device required */
#define EBUSY 16    /* Mount device busy */
#define EEXIST 17   /* File exists */
#define EXDEV 18    /* Cross-device link */
#define ENODEV 19   /* No such device */
#define ENOTDIR 20  /* Not a directory */
#define EISDIR 21   /* Is a directory */
#define EINVAL 22   /* Invalid argument */
#define ENFILE 23   /* Too many open files in system */
#define EMFILE 24   /* Too many open files */
#define ENOTTY 25   /* Not a typewriter */
#define ETXTBSY 26  /* Text file busy */
#define EFBIG 27    /* File too large */
#define ENOSPC 28   /* No space left on device */
#define ESPIPE 29   /* Illegal seek */
#define EROFS 30    /* Read only file system */
#define EMLINK 31   /* Too many links */
#define EPIPE 32    /* Broken pipe */
#define EDOM 33     /* Math arg out of domain of func */
#define ERANGE 34   /* Math result not representable */
#define ENOMSG 35   /* No message of desired type */
#define EIDRM 36    /* Identifier removed */
#define ECHRNG 37   /* Channel number out of range */
#define EL2NSYNC 38 /* Level 2 not synchronized */
#define EL3HLT 39   /* Level 3 halted */
#define EL3RST 40   /* Level 3 reset */
#define ELNRNG 41   /* Link number out of range */
#define EUNATCH 42  /* Protocol driver not attached */
#define ENOCSI 43   /* No CSI structure available */
#define EL2HLT 44   /* Level 2 halted */
#define EDEADLK 45  /* Deadlock condition */
#define ENOLCK 46   /* No record locks available */
#define EBADE 50    /* Invalid exchange */
#define EBADR 51    /* Invalid request descriptor */
#define EXFULL 52   /* Exchange full */
#define ENOANO 53   /* No anode */
#define EBADRQC 54  /* Invalid request code */
#define EBADSLT 55  /* Invalid slot */
#define EDEADLOCK 56    /* File locking deadlock error */
#define EBFONT 57   /* Bad font file fmt */
#define ENOSTR 60   /* Device not a stream */
#define ENODATA 61  /* No data (for no delay io) */
#define ETIME 62    /* Timer expired */
#define ENOSR 63    /* Out of streams resources */
#define ENONET 64   /* Machine is not on the network */
#define ENOPKG 65   /* Package not installed */
#define EREMOTE 66  /* The object is remote */
#define ENOLINK 67  /* The link has been severed */
#define EADV 68     /* Advertise error */
#define ESRMNT 69   /* Srmount error */
#define ECOMM 70    /* Communication error on send */
#define EPROTO 71   /* Protocol error */
#define EMULTIHOP 74    /* Multihop attempted */
#define ELBIN 75    /* Inode is remote (not really error) */
#define EDOTDOT 76  /* Cross mount point (not really error) */
#define EBADMSG 77  /* Trying to read unreadable message */
#define EFTYPE 79   /* Inappropriate file type or format */
#define ENOTUNIQ 80 /* Given log. name not unique */
#define EBADFD 81   /* f.d. invalid for this operation */
#define EREMCHG 82  /* Remote address changed */
#define ELIBACC 83  /* Can't access a needed shared lib */
#define ELIBBAD 84  /* Accessing a corrupted shared lib */
#define ELIBSCN 85  /* .lib section in a.out corrupted */
#define ELIBMAX 86  /* Attempting to link in too many libs */
#define ELIBEXEC 87 /* Attempting to exec a shared library */
#define ENOSYS 88   /* Function not implemented */
#define ENMFILE 89      /* No more files */
#define ENOTEMPTY 90    /* Directory not empty */
#define ENAMETOOLONG 91 /* File or path name too long */
#define ELOOP 92    /* Too many symbolic links */
#define EOPNOTSUPP 95   /* Operation not supported on transport endpoint */
#define EPFNOSUPPORT 96 /* Protocol family not supported */
#define ECONNRESET 104  /* Connection reset by peer */
#define ENOBUFS 105 /* No buffer space available */
#define EAFNOSUPPORT 106 /* Address family not supported by protocol family */
#define EPROTOTYPE 107  /* Protocol wrong type for socket */
#define ENOTSOCK 108    /* Socket operation on non-socket */
#define ENOPROTOOPT 109 /* Protocol not available */
#define ESHUTDOWN 110   /* Can't send after socket shutdown */
#define ECONNREFUSED 111    /* Connection refused */
#define EADDRINUSE 112      /* Address already in use */
#define ECONNABORTED 113    /* Connection aborted */
#define ENETUNREACH 114     /* Network is unreachable */
#define ENETDOWN 115        /* Network interface is not configured */
#define ETIMEDOUT 116       /* Connection timed out */
#define EHOSTDOWN 117       /* Host is down */
#define EHOSTUNREACH 118    /* Host is unreachable */
#define EINPROGRESS 119     /* Connection already in progress */
#define EALREADY 120        /* Socket already connected */
#define EDESTADDRREQ 121    /* Destination address required */
#define EMSGSIZE 122        /* Message too long */
#define EPROTONOSUPPORT 123 /* Unknown protocol */
#define ESOCKTNOSUPPORT 124 /* Socket type not supported */
#define	EMEDIUMTYPE	124	
#define EADDRNOTAVAIL 125   /* Address not available */
#define ENETRESET 126
#define EISCONN 127     /* Socket is already connected */
#define ENOTCONN 128        /* Socket is not connected */
#define ETOOMANYREFS 129
#define EPROCLIM 130
#define EUSERS 131
#define EDQUOT 132
#define ESTALE 133
#define ENOTSUP 134     /* Not supported */
#define ENOMEDIUM 135   /* No medium (in tape drive) */
#define ENOSHARE 136    /* No such host or network path */
#define ECASECLASH 137  /* Filename exists with different case */
#define EILSEQ 138
#define EOVERFLOW 139   /* Value too large for defined data type */

#define EWOULDBLOCK EAGAIN  /* Operation would block */

#define __ELASTERROR 2000   /* Users can add values starting here */

#endif
