#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <mach/mach_init.h>
#include <mach/mach_traps.h>
#include <mach/mach_types.h>
#include <mach/mach_vm.h>
#include <mach/vm_map.h>

kern_return_t merror;

bool isRoot() {
  if (getuid()) {
    if (geteuid()) {
      return false;
    }
  }

  return true;
}

bool isProcessValid(mach_port_t process) {
  return MACH_PORT_VALID(process);
}

kern_return_t isNoError() {
  return merror == KERN_SUCCESS;
}

mach_port_t getProcess(int pid) {
  mach_port_t _result;
  merror = task_for_pid(mach_task_self(), pid, &_result);
  return _result;
}

uintptr_t getBaseAddressByRegion(mach_port_t process, int region) {
  kern_return_t lerror = KERN_SUCCESS;
  vm_address_t address = 0;
  vm_size_t size = 0;
  uint32_t depth = 1;

  int region_id = 0;

  uintptr_t _result;

  while (true) {
    struct vm_region_submap_info_64 info;
    mach_msg_type_number_t count = VM_REGION_SUBMAP_INFO_COUNT_64;

    lerror = vm_region_recurse_64(process, &address, &size, &depth, (vm_region_info_64_t)&info, &count);

    if (lerror == KERN_INVALID_ADDRESS){
      break;
    }

    if (info.is_submap) {
      depth++;
    } else {
      if (region_id++ == region) {
        _result = (uintptr_t)address;
      }

      address += size;
    }
  }

  return _result;
}

void * readAddress(mach_port_t process, uintptr_t address, int size) {
  void **bytes;
  unsigned int _result_size;
  vm_offset_t dataPointer = 0;
  merror = vm_read(process, address, size, &dataPointer, &_result_size);
  bytes = (void *)dataPointer;
  return *bytes;
}

void writeAddress(mach_port_t process, uintptr_t address, int size, void * value) {
  merror = vm_write(process, address, (vm_address_t)value, size);
}
