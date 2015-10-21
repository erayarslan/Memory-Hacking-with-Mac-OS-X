#include <mach/mach.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define PID 4092
#define ADDR 0x7fff5c7d3b5c

int value = 123456;

int main(int argc, char *argv[]) {
  mach_port_t process_to_write;
  kern_return_t error;

  if(getuid() && geteuid()) {
    printf("You need to be root to vm_write!\n");
  } else{
    error = task_for_pid(mach_task_self(), PID, &process_to_write);

    if ((error != KERN_SUCCESS) || !MACH_PORT_VALID(process_to_write)) {
      printf("Error getting the process!\n");
    }

    if(vm_write(process_to_write, (vm_address_t) ADDR, (vm_address_t)&value, sizeof(value))) {
      printf("Ooops!\n");
    }

    printf("Done! :-)\n");
  }

  return 0;
}
