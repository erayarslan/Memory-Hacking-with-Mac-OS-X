#include <stdio.h>
#include <stdint.h>
#include "melkor.c"

int pid;

int offset = 0x14;
int baseOffset = 0x7ffb30;
int region = 16;

int main() {
  if (isRoot()) {
    printf("[+] pid: ");
    scanf("%d", &pid);

    mach_port_t process = getProcess(pid);

    if (isNoError() && isProcessValid(process)) {
      uintptr_t baseAddress = getBaseAddressByRegion(process, 16);
      uintptr_t pointerAddress = (uintptr_t)readAddress(process, baseAddress + baseOffset, sizeof(uintptr_t));
      int target = (int)readAddress(process, pointerAddress - offset, sizeof(int));

      printf("[x] result: %d\n", target);
    }
  }

  return 0;
}
