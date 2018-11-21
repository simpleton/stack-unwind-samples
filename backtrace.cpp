#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#define UNW_LOCAL_ONLY
#include <cxxabi.h>
#include <execinfo.h>
#include <libunwind.h>

#include <stdio.h>
#include <stdlib.h>

#define LOOP_SIZE 6

void gcc_bt() {

  void *pc0 = __builtin_return_address(0);
  void *pc1 = __builtin_return_address(1);
  void *pc2 = __builtin_return_address(2);
  void *pc3 = __builtin_return_address(3);
  void *pc4 = __builtin_return_address(4);
  void *pc5 = __builtin_return_address(5);
  void *pc6 = __builtin_return_address(6);
  void *pc7 = __builtin_return_address(7);

  printf("Frame 0: PC=%p\n", pc0);
  printf("Frame 1: PC=%p\n", pc1);
  printf("Frame 2: PC=%p\n", pc2);
  printf("Frame 3: PC=%p\n", pc3);
  printf("Frame 4: PC=%p\n", pc4);
  printf("Frame 5: PC=%p\n", pc5);
  printf("Frame 6: PC=%p\n", pc6);
  printf("Frame 7: PC=%p\n", pc7);
}

void glibc_bt() {
  void *array[LOOP_SIZE+3];
  size_t size, i;
  char **strings;

  size = backtrace(array, LOOP_SIZE+3);
  strings = backtrace_symbols(array, size);

  for (i = 0; i < size; i++) {
    printf("%p : %s\n", array[i], strings[i]);
  }

  free(strings); // malloced by backtrace_symbols
}

void libunwind_bt() {
  unw_cursor_t cursor;
  unw_context_t context;

  unw_getcontext(&context);
  unw_init_local(&cursor, &context);

  int n = 0;
  while (unw_step(&cursor)) {
    unw_word_t ip, sp, off;

    unw_get_reg(&cursor, UNW_REG_IP, &ip);
    unw_get_reg(&cursor, UNW_REG_SP, &sp);

    char symbol[256] = {"<unknown>"};
    char *name = symbol;

    if (!unw_get_proc_name(&cursor, symbol, sizeof(symbol), &off)) {
      int status;
      if ((name = abi::__cxa_demangle(symbol, NULL, NULL, &status)) == 0)
        name = symbol;
    }

    printf("#%-2d 0x%016" PRIxPTR " sp=0x%016" PRIxPTR " %s + 0x%" PRIxPTR "\n",
           n++, static_cast<uintptr_t>(ip), static_cast<uintptr_t>(sp), name,
           static_cast<uintptr_t>(off));

    if (name != symbol)
      free(name);
  }
}

int foo(int n) {
  if (n == 0) {
    printf("## gcc backtrace support:\n");
    gcc_bt();
    printf("\n");

    printf("## glibc backtrace support:\n");
    glibc_bt();
    printf("\n");

    printf("## Libunwind:\n");
    libunwind_bt();
    printf("\n");

    return 1;
  } else {
    return n * foo(n - 1);
  }
}

int main() {
  foo(LOOP_SIZE);
  return 0;
}
