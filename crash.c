#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#ifdef _WIN32
  #ifndef _WIN32_WINNT
    #define _WIN32_WINNT 0x0601             // Windows 7+, needed for GetTickCount64
  #endif
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
  #include <io.h>
  #include <fcntl.h>
#else
  #include <unistd.h>
  #include <sys/wait.h>
  #include <sys/utsname.h>
  #include <sys/statvfs.h>
  #include <execinfo.h>
  #ifdef __linux__
    #include <sys/sysinfo.h>
  #endif
#endif

#define MAX_LINES 200
#define LINE_LEN  1024
#define BT_BUF_SIZE 200

#define GB (1024.0 * 1024.0 * 1024.0)

// Ring buffer of the last MAX_LINES lines the program printed
static char lines[MAX_LINES][LINE_LEN];
static int line_count = 0;


static void write_header(FILE *f, const char *title)
{
  fprintf(f, "\n=== %s ===\n", title);
}

static void write_timestamp(FILE *f)
{
  char buf[64];
  time_t now = time(NULL);
  strftime(buf, sizeof buf, "%Y-%m-%d %H:%M:%S", localtime(&now));
  fprintf(f, "Device information report\nGenerated: %s\n", buf);
}

#ifdef _WIN32
/* ------------------------------- Windows -------------------------------- */

static void collect_os(FILE *f)
{
  char name[MAX_COMPUTERNAME_LENGTH + 1];
  DWORD size = sizeof name;
  SYSTEM_INFO si;

  write_header(f, "System");
  if (GetComputerNameA(name, &size))
    fprintf(f, "Hostname     : %s\n", name);

  /* GetVersionEx lies on Windows 8.1+, the registry doesn't */
  HKEY key;
  if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                    "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                    0, KEY_READ, &key) == ERROR_SUCCESS) {
    char product[128] = "Windows", build[32] = "?";
    DWORD psize = sizeof product, bsize = sizeof build;
    RegQueryValueExA(key, "ProductName", NULL, NULL, (LPBYTE)product, &psize);
    RegQueryValueExA(key, "CurrentBuildNumber", NULL, NULL, (LPBYTE)build, &bsize);
    RegCloseKey(key);
    fprintf(f, "OS           : %s (build %s)\n", product, build);
  }

  GetNativeSystemInfo(&si);
  fprintf(f, "Architecture : %s\n",
          si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ? "x86_64" :
          si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64 ? "ARM64" :
          si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL ? "x86" : "Other");
  fprintf(f, "Uptime       : %.1f hours\n", GetTickCount64() / 3600000.0);
}

static void collect_cpu(FILE *f)
{
  SYSTEM_INFO si;
  char model[256] = "Unknown";
  DWORD size = sizeof model;
  HKEY key;

  write_header(f, "CPU");
  if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                    "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
                    0, KEY_READ, &key) == ERROR_SUCCESS) {
    RegQueryValueExA(key, "ProcessorNameString", NULL, NULL, (LPBYTE)model, &size);
    RegCloseKey(key);
  }
  GetNativeSystemInfo(&si);
  fprintf(f, "Model        : %s\n", model);
  fprintf(f, "Logical cores: %lu\n", (unsigned long)si.dwNumberOfProcessors);
}

static void collect_memory(FILE *f)
{
  MEMORYSTATUSEX m;
  m.dwLength = sizeof m;
  write_header(f, "Memory");
  if (GlobalMemoryStatusEx(&m)) {
    fprintf(f, "Total RAM    : %.2f GB\n", m.ullTotalPhys / GB);
    fprintf(f, "Free RAM     : %.2f GB\n", m.ullAvailPhys / GB);
    fprintf(f, "Usage        : %lu%%\n", (unsigned long)m.dwMemoryLoad);
  }
}

static void collect_disk(FILE *f)
{
  ULARGE_INTEGER avail, total, free_b;
  write_header(f, "Disk (C:\\)");
  if (GetDiskFreeSpaceExA("C:\\", &avail, &total, &free_b)) {
    fprintf(f, "Total        : %.2f GB\n", total.QuadPart / GB);
    fprintf(f, "Free         : %.2f GB\n", free_b.QuadPart / GB);
  }
}

#else
/* ---------------------------- Linux / macOS ----------------------------- */

static void collect_os(FILE *f)
{
  struct utsname u;
  char host[256];

  write_header(f, "System");
  if (gethostname(host, sizeof host) == 0)
    fprintf(f, "Hostname     : %s\n", host);
  if (uname(&u) == 0) {
    fprintf(f, "OS           : %s\n", u.sysname);
    fprintf(f, "Kernel       : %s\n", u.release);
    fprintf(f, "Version      : %s\n", u.version);
    fprintf(f, "Architecture : %s\n", u.machine);
  }

  /* Distribution name (Linux) */
  FILE *os = fopen("/etc/os-release", "r");
  if (!os) os = fopen("/usr/lib/os-release", "r");   // fallback per the os-release spec
  if (os) {
    char line[256];
    while (fgets(line, sizeof line, os)) {
      if (strncmp(line, "PRETTY_NAME=", 12) == 0) {
        char *v = line + 12;
        v[strcspn(v, "\n")] = '\0';
        if (*v == '"') { v++; v[strcspn(v, "\"")] = '\0'; }
        fprintf(f, "Distribution : %s\n", v);
        break;
      }
    }
    fclose(os);
  }

#ifdef __linux__
  struct sysinfo si;
  if (sysinfo(&si) == 0)
    fprintf(f, "Uptime       : %.1f hours\n", si.uptime / 3600.0);
  else
    perror("Couldn't retrieve sysinfo");
#endif
}

static void collect_cpu(FILE *f)
{
  write_header(f, "CPU");
  FILE *cpu = fopen("/proc/cpuinfo", "r");
  if (cpu) {
    char line[512];
    while (fgets(line, sizeof line, cpu)) {
      if (strncmp(line, "model name", 10) == 0) {
        char *v = strchr(line, ':');
        if (v) {
          v += 2;
          v[strcspn(v, "\n")] = '\0';
          fprintf(f, "Model        : %s\n", v);
        }
        break;
      }
    }
    fclose(cpu);
  }
  fprintf(f, "Logical cores: %ld\n", sysconf(_SC_NPROCESSORS_ONLN));
}

static void collect_memory(FILE *f)
{
  write_header(f, "Memory");
#ifdef __linux__
  struct sysinfo si;
  if (sysinfo(&si) == 0) {
    double total = (double)si.totalram * si.mem_unit;
    double free_ = (double)si.freeram * si.mem_unit;
    fprintf(f, "Total RAM    : %.2f GB\n", total / GB);
    fprintf(f, "Free RAM     : %.2f GB\n", free_ / GB);
    fprintf(f, "Swap total   : %.2f GB\n", (double)si.totalswap * si.mem_unit / GB);
  }
#else
  long pages = sysconf(_SC_PHYS_PAGES), psize = sysconf(_SC_PAGESIZE);
  if (pages > 0 && psize > 0)
    fprintf(f, "Total RAM    : %.2f GB\n", (double)pages * psize / GB);
#endif
}

static void collect_disk(FILE *f)
{
  struct statvfs s;
  write_header(f, "Disk (/)");
  if (statvfs("/", &s) == 0) {
    double total = (double)s.f_blocks * s.f_frsize;
    double avail = (double)s.f_bavail * s.f_frsize;
    fprintf(f, "Total        : %.2f GB\n", total / GB);
    fprintf(f, "Available    : %.2f GB\n", avail / GB);
    fprintf(f, "Used         : %.1f%%\n", total > 0 ? 100.0 * (total - avail) / total : 0.0);
  }
}
#endif


/* ------------------------- shared watcher logic ------------------------- */

// Parent: echo the child's output live and keep the last MAX_LINES lines
static void pump_output(FILE *in)
{
  char buf[LINE_LEN];
  while (fgets(buf, sizeof buf, in)) {
    fputs(buf, stdout);
    fflush(stdout);
    strcpy(lines[line_count++ % MAX_LINES], buf);
  }
}

static void write_crash_log(const char *reason)
{
  FILE *f = fopen("crash.log", "w");
  if (f == NULL)
    perror("Couldn't open crash.log");
  FILE *out = f ? f : stderr;

  fprintf(out, "Crashed with %s. Last lines:\n\n", reason);
  // oldest line first; once the ring wrapped, it starts at line_count % MAX_LINES
  int start = line_count > MAX_LINES ? line_count - MAX_LINES : 0;
  for (int i = start; i < line_count; i++)
    fputs(lines[i % MAX_LINES], out);
  fputs("\n", out);

  // Log system information
  write_timestamp(out);
  collect_os(out);
  collect_cpu(out);
  collect_memory(out);
  collect_disk(out);

  if (f) fclose(f);
  fprintf(stderr, "\n*** Program crashed, check logs***\n");
}


#ifdef _WIN32
/* --------------------------- Windows watcher ---------------------------- */
/*
  Windows has no fork(), so the watcher re-launches this same exe with its
  stdout/stderr going into a pipe. The env var tells the relaunched copy that
  it is the child. Anything in main() before crash_watcher_init() runs twice.
*/

#define CHILD_ENV "CRASH_WATCHER_CHILD"

static void err_write(const char *s)
{
  DWORD written;
  WriteFile(GetStdHandle(STD_ERROR_HANDLE), s, (DWORD)strlen(s), &written, NULL);
}

static const char *exception_name(DWORD code)
{
  switch (code) {
    case EXCEPTION_ACCESS_VIOLATION:      return "Access violation";
    case EXCEPTION_STACK_OVERFLOW:        return "Stack overflow";
    case EXCEPTION_INT_DIVIDE_BY_ZERO:    return "Integer divide by zero";
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:    return "Float divide by zero";
    case EXCEPTION_ILLEGAL_INSTRUCTION:   return "Illegal instruction";
    case EXCEPTION_PRIV_INSTRUCTION:      return "Privileged instruction";
    case EXCEPTION_IN_PAGE_ERROR:         return "In-page error";
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: return "Array bounds exceeded";
    case EXCEPTION_DATATYPE_MISALIGNMENT: return "Datatype misalignment";
    case EXCEPTION_BREAKPOINT:            return "Breakpoint";
    case 0xC0000409:                      return "Stack buffer overrun / fast fail";
    case 0xC0000374:                      return "Heap corruption";
    case 0x40000015:                      return "Abort";  // STATUS_FATAL_APP_EXIT
    default:                              return "Unknown exception";
  }
}

// Exit codes that mean the process died from an exception rather than exit()
static int is_crash_code(DWORD code)
{
  if (code == 0xC000013A)               // STATUS_CONTROL_C_EXIT: user hit Ctrl+C
    return 0;
  return (code & 0xF0000000) == 0xC0000000
      || code == EXCEPTION_BREAKPOINT
      || code == 0x40000015;
}

// Same shape as glibc's backtrace_symbols: module(+offset) [addr]
// Look the offset up with addr2line (-g builds) or the linker map file.
static void write_frame(DWORD64 pc)
{
  char buf[MAX_PATH + 64], path[MAX_PATH];
  const char *module = "???";
  HMODULE mod = NULL;

  if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                         GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                         (LPCSTR)(ULONG_PTR)pc, &mod)
      && GetModuleFileNameA(mod, path, sizeof path)) {
    const char *slash = strrchr(path, '\\');
    module = slash ? slash + 1 : path;
  }
  snprintf(buf, sizeof buf, "%s(+0x%llx) [0x%llx]\n", module,
           (unsigned long long)(pc - (DWORD64)(ULONG_PTR)mod), (unsigned long long)pc);
  err_write(buf);
}

// Child: runs on the crashing thread, writes the backtrace into the pipe
static LONG WINAPI crash_exception_filter(EXCEPTION_POINTERS *ep)
{
  static volatile LONG entered = 0;
  char buf[128];

  // A corrupt stack can fault again while we unwind it, don't recurse
  if (InterlockedExchange(&entered, 1))
    return EXCEPTION_EXECUTE_HANDLER;

  snprintf(buf, sizeof buf, "\n%s (0x%08lX) at %p\n--- backtrace ---\n",
           exception_name(ep->ExceptionRecord->ExceptionCode),
           (unsigned long)ep->ExceptionRecord->ExceptionCode,
           ep->ExceptionRecord->ExceptionAddress);
  err_write(buf);

#if defined(__x86_64__) || defined(__aarch64__)
  // Walk the stack from the faulting frame using the exe/dll unwind tables
  // (.pdata), so it also works without frame pointers at -O3.
  CONTEXT ctx = *ep->ContextRecord;
  #ifdef __x86_64__
    #define CTX_PC(c) (c).Rip
  #else
    #define CTX_PC(c) (c).Pc
  #endif
  for (int i = 0; i < BT_BUF_SIZE && CTX_PC(ctx) != 0; i++) {
    DWORD64 pc = CTX_PC(ctx), image_base;
    write_frame(pc);

    PRUNTIME_FUNCTION fn = RtlLookupFunctionEntry(pc, &image_base, NULL);
    if (fn) {
      PVOID handler_data;
      DWORD64 establisher;
      RtlVirtualUnwind(UNW_FLAG_NHANDLER, image_base, pc, fn, &ctx,
                       &handler_data, &establisher, NULL);
    } else {
      // Leaf function without unwind info: return address is right there
  #ifdef __x86_64__
      ctx.Rip = *(DWORD64 *)ctx.Rsp;
      ctx.Rsp += 8;
  #else
      ctx.Pc = ctx.Lr;
  #endif
    }
    if (CTX_PC(ctx) == pc)              // unwinder made no progress
      break;
  }
  #undef CTX_PC
#else
  // 32-bit x86 has no unwind tables, only report where it crashed
  write_frame((DWORD64)(ULONG_PTR)ep->ExceptionRecord->ExceptionAddress);
#endif

  // Terminate with the exception code as exit code, no WER dialog
  return EXCEPTION_EXECUTE_HANDLER;
}

// abort() doesn't raise an SEH exception, turn it into one to get a backtrace
static void abort_handler(int sig)
{
  (void)sig;
  RaiseException(0x40000015, EXCEPTION_NONCONTINUABLE, 0, NULL);
}

static void install_crash_handlers(void)
{
  ULONG guarantee = 64 * 1024;          // stack left for the filter on stack overflow
  SetThreadStackGuarantee(&guarantee);
  SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
  signal(SIGABRT, abort_handler);
  SetUnhandledExceptionFilter(crash_exception_filter);
}

// Call first thing in main(). Returns only in the child (your real program).
void crash_watcher_init(void)
{
  char flag[2];
  if (GetEnvironmentVariableA(CHILD_ENV, flag, sizeof flag) > 0) {
    // child: continue as the normal program
    SetEnvironmentVariableA(CHILD_ENV, NULL);   // don't leak into processes we spawn
    install_crash_handlers();
    // The Windows CRT treats _IOLBF as full buffering, so go unbuffered to not lose lines
    setvbuf(stdout, NULL, _IONBF, 0);
    return;
  }

  SECURITY_ATTRIBUTES sa = { sizeof sa, NULL, TRUE };
  HANDLE rd, wr;
  if (!CreatePipe(&rd, &wr, &sa, 0)) {    // if it fails, just run without watching
    install_crash_handlers();
    return;
  }
  SetHandleInformation(rd, HANDLE_FLAG_INHERIT, 0);   // only the write end goes to the child

  wchar_t exe[MAX_PATH];
  GetModuleFileNameW(NULL, exe, MAX_PATH);
  wchar_t *cmdline = _wcsdup(GetCommandLineW());     // CreateProcessW may write to it

  STARTUPINFOW si;
  PROCESS_INFORMATION pi;
  memset(&si, 0, sizeof si);
  si.cb = sizeof si;
  si.dwFlags = STARTF_USESTDHANDLES;
  si.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);
  si.hStdOutput = wr;
  si.hStdError  = wr;

  SetEnvironmentVariableA(CHILD_ENV, "1");
  BOOL ok = CreateProcessW(exe, cmdline, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
  SetEnvironmentVariableA(CHILD_ENV, NULL);
  free(cmdline);
  CloseHandle(wr);                        // otherwise we never see EOF
  if (!ok) {
    CloseHandle(rd);
    install_crash_handlers();
    return;
  }
  CloseHandle(pi.hThread);

  // parent: ignore Ctrl+C ourselves so we outlive the child and report its exit
  SetConsoleCtrlHandler(NULL, TRUE);

  int fd = _open_osfhandle((intptr_t)rd, _O_RDONLY | _O_TEXT);
  FILE *in = fd >= 0 ? _fdopen(fd, "r") : NULL;
  if (in) {
    pump_output(in);
    fclose(in);
  } else {
    CloseHandle(rd);
  }

  DWORD code = 1;
  WaitForSingleObject(pi.hProcess, INFINITE);
  GetExitCodeProcess(pi.hProcess, &code);
  CloseHandle(pi.hProcess);

  if (is_crash_code(code)) {
    char reason[128];
    snprintf(reason, sizeof reason, "exception 0x%08lX (%s)",
             (unsigned long)code, exception_name(code));
    write_crash_log(reason);
  }
  exit((int)code);
}

#else
/* ------------------------ Linux / macOS watcher ------------------------- */

// Child: runs on the crashing thread, writes the backtrace into the pipe
static void crash_signal_handler(int sig)
{
  static const char msg[] = "\n--- backtrace ---\n";
  void *buffer[BT_BUF_SIZE];
  int nptrs = backtrace(buffer, BT_BUF_SIZE);
  write(STDERR_FILENO, msg, sizeof msg - 1);
  backtrace_symbols_fd(buffer, nptrs, STDERR_FILENO);
  raise(sig);                             // SA_RESETHAND: dies with the default action
}

static void install_crash_handlers(void)
{
  // Separate stack so stack overflows can still be reported
  static char altstack[64 * 1024];
  stack_t ss = { .ss_sp = altstack, .ss_size = sizeof altstack, .ss_flags = 0 };
  sigaltstack(&ss, NULL);

  // backtrace() loads libgcc lazily, do it now and not inside the handler
  void *dummy[1];
  backtrace(dummy, 1);

  struct sigaction sa;
  memset(&sa, 0, sizeof sa);
  sa.sa_handler = crash_signal_handler;
  sa.sa_flags = SA_ONSTACK | SA_RESETHAND;
  sigemptyset(&sa.sa_mask);
  int sigs[] = { SIGSEGV, SIGBUS, SIGFPE, SIGILL, SIGABRT };
  for (size_t i = 0; i < sizeof sigs / sizeof *sigs; i++)
    sigaction(sigs[i], &sa, NULL);
}

// Call first thing in main(). Returns only in the child (your real program).
void crash_watcher_init(void)
{
  int fd[2];
  if (pipe(fd) < 0) return;               // if it fails, just run without watching

  pid_t pid = fork();
  if (pid < 0) return;

  if (pid == 0) {                         // child: continue as the normal program
    // dup2 makes two args same thing, old->new
    dup2(fd[1], STDOUT_FILENO);
    dup2(fd[1], STDERR_FILENO);
    close(fd[0]); close(fd[1]);
    setvbuf(stdout, NULL, _IOLBF, 0);   // don't lose buffered lines on crash
    install_crash_handlers();
    return;
  }

  close(fd[1]);
  FILE *in = fdopen(fd[0], "r");
  pump_output(in);
  fclose(in);

  int status;
  waitpid(pid, &status, 0);
  if (WIFSIGNALED(status)) {
    char reason[128];
    snprintf(reason, sizeof reason, "signal %d (%s)",
             WTERMSIG(status), strsignal(WTERMSIG(status)));
    write_crash_log(reason);
    exit(128 + WTERMSIG(status));
  }
  exit(WIFEXITED(status) ? WEXITSTATUS(status) : 1);
}
#endif


void func2() {

}

int main(void) {
  crash_watcher_init();
  // ... your program, printf as usual
  printf("hiii \n");
  printf("hiii \n");
  printf("hiii \n");
  printf("hiii \n");
  printf("hiii \n");
  func2();
  strcat(NULL, "");

  return 0;
}
