#include <stdio.h>
#include <mach/mach_init.h>
#include <sys/sysctl.h>
#include <mach/mach_vm.h>
#include <unistd.h>

int main() {
  mach_port_t process_to_write;
  kern_return_t error;
  int pid;

  if(getuid() && geteuid()) {
    printf("You need to be root to vm_write!\n");
  } else{
    printf("PID: ");
    scanf("%d", &pid);

    error = task_for_pid(mach_task_self(), pid, &process_to_write);

    if ((error != KERN_SUCCESS) || !MACH_PORT_VALID(process_to_write)) {
      printf("Error getting the process!\n");
    }

    mach_port_name_t task;
    vm_map_offset_t vmoffset;
    vm_map_size_t vmsize;
    uint32_t nesting_depth = 0;
    struct vm_region_submap_info_64 vbr;
    mach_msg_type_number_t vbrcount = 16;
    kern_return_t kr;

    if ((kr = mach_vm_region_recurse(process_to_write, &vmoffset, &vmsize,
                                    &nesting_depth,
                                    (vm_region_recurse_info_t)&vbr,
                                    &vbrcount)) != KERN_SUCCESS)
    {
      printf("Error");
    }

    printf("%p\n", (void *) (uintptr_t)vmoffset);

    int sz;
    int sz2;


    vm_offset_t dataPointer = 0;
    vm_offset_t dataPointer2 = 0;

    void **bytes;
    void **bytes2;

    error = vm_read(process_to_write, (vm_address_t)(0x7FFE43081B40 + vmoffset), sizeof(vm_address_t), &dataPointer, &sz);

    if (error == KERN_SUCCESS) {
      bytes = (void *)dataPointer;
      printf("%p\n", *bytes);

      error = vm_read(process_to_write, (void *) (uintptr_t)(*bytes-0x14), sizeof(int), &dataPointer2, &sz2);

      if (error == KERN_SUCCESS) {
        bytes2 = (void *)dataPointer2;
        printf("%d\n", *bytes2);
      }
    }
  }

  return 0;
}
