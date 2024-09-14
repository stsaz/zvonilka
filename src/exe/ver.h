/** zvonilka: executor */

#if defined FF_WIN
	#define OS_STR  "windows"
#elif defined FF_BSD
	#define OS_STR  "bsd"
#elif defined FF_APPLE
	#define OS_STR  "macos"
#else
	#define OS_STR  "linux"
#endif

#if defined FF_AMD64
	#define CPU_STR  "amd64"
#elif defined FF_X86
	#define CPU_STR  "x86"
#elif defined FF_ARM64
	#define CPU_STR  "arm64"
#elif defined FF_ARM
	#define CPU_STR  "arm"
#endif

static void version_print()
{
	ffstderr_fmt("zvonilka v%s (" OS_STR "-" CPU_STR ")\n"
		, x->core->version_str);
}
