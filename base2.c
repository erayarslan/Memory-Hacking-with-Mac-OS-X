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

    kern_return_t krc = KERN_SUCCESS;
    vm_address_t address = 0;
    vm_size_t size = 0;
    uint32_t depth = 1;
    while (1) {
        struct vm_region_submap_info_64 info;
        mach_msg_type_number_t count = VM_REGION_SUBMAP_INFO_COUNT_64;
        krc = vm_region_recurse_64(process_to_write, &address, &size, &depth, (vm_region_info_64_t)&info, &count);
        if (krc == KERN_INVALID_ADDRESS){
            break;
        }
        if (info.is_submap){
            depth++;
        }
        else {
            //do stuff
            printf ("Found region: %p to %p\n", (uint32_t)address, (uint32_t)address+size);
            address += size;
        }
    }
    /*

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

    */
  }

  return 0;
}
