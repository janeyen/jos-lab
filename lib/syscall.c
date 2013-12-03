// System call stubs.

#include <inc/syscall.h>
#include <inc/lib.h>

#define asmcode(...) #__VA_ARGS__

enum {
    Unknown,
    Support,
    Unsupport,
};
static int supportFastSyscall = Unknown;

int32_t
syscall_fastcall(int num, int check, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4)
{
    int32_t ret = 0;

    // use ECX temporarily to push ReturnBack address into stack (dESP = -4)
    asm volatile
    (
        asmcode(
           lea ReturnBack, %%ecx;
           push %%ecx;
        )
        ::: "ecx"
    );

    // protect ECX, EDX, and EBP, then do fast-syscall (dESP = -12)
    asm volatile
    (
        asmcode(
            push %%ecx;
            push %%edx;
            push %%ebp;
            mov  %%esp, %%ebp;
            sysenter;
        )
        :
        : "a"(num),
          "d"(a1), "c"(a2), "b"(a3), "D"(a4)
        : "memory"
    );

    // it should be back to here:
    asm volatile ("ReturnBack:");

    // recover the registers and stack (dESP = 0)
    asm volatile
    (
        asmcode
        (
            pop %%ebp;
            pop %%edx;
            pop %%ecx;
            add $4, %%esp;
        )
        : "=a"(ret)
        :: "memory"
    );

    if(check && ret > 0)
        panic("syscall %d returned %d (> 0)", num, ret);

    return ret;
}

static inline int32_t
syscall_trap(int num, int check, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5)
{
	int32_t ret;

	// Generic system call: pass system call number in AX,
	// up to five parameters in DX, CX, BX, DI, SI.
	// Interrupt kernel with T_SYSCALL.
	//
	// The "volatile" tells the assembler not to optimize
	// this instruction away just because we don't use the
	// return value.
	//
	// The last clause tells the assembler that this can
	// potentially change the condition codes and arbitrary
	// memory locations.

	asm volatile("int %1\n"
		: "=a" (ret)
		: "i" (T_SYSCALL),
		  "a" (num),
		  "d" (a1),
		  "c" (a2),
		  "b" (a3),
		  "D" (a4),
		  "S" (a5)
		: "cc", "memory");

	if(check && ret > 0)
		panic("syscall %d returned %d (> 0)", num, ret);

	return ret;
}

static inline int32_t
syscall(int num, int check, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5)
{
    // make sure the cpu supports fast syscall
    if(supportFastSyscall == Unknown)
        supportFastSyscall = syscall_trap(SYS_sysenter, 0, 0, 0, 0, 0, 0) ? Support : Unsupport;

    if(supportFastSyscall == Unsupport || a5)
    {
        // normal trap syscall
        return syscall_trap(num, check, a1, a2, a3, a4, a5);
    }
    else
    {
        // just 4 args, and we can use fast syscall
        return syscall_fastcall(num, check, a1, a2, a3, a4);
    }
}


void
sys_cputs(const char *s, size_t len)
{
	syscall(SYS_cputs, 0, (uint32_t)s, len, 0, 0, 0);
}

int
sys_cgetc(void)
{
	return syscall(SYS_cgetc, 0, 0, 0, 0, 0, 0);
}

int
sys_env_destroy(envid_t envid)
{
	return syscall(SYS_env_destroy, 1, envid, 0, 0, 0, 0);
}

envid_t
sys_getenvid(void)
{
	 return syscall(SYS_getenvid, 0, 0, 0, 0, 0, 0);
}

bool
sys_sysenter(void)
{
    return syscall(SYS_sysenter, 0, 0, 0, 0, 0, 0);
}
