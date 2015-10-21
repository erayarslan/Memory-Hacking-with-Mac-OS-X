#include <stdio.h>
#include <stdlib.h>
#include <mach/mach.h>

int main() {

   kern_return_t kern_return;
   mach_port_t task;

   int pid = 0;
   printf("Enter PID to look-up: ");
   scanf("%d", &pid);

   // Need to run this program as root (i.e. sudo) in order for this to work
   kern_return = task_for_pid(mach_task_self(), pid, &task);
   if (kern_return != KERN_SUCCESS)
   {
      printf("task_for_pid() failed, error %d - %s\n", kern_return, mach_error_string(kern_return));
      exit(1);
   }

   kern_return_t kret;
   vm_region_basic_info_data_t info;
   vm_size_t size;
   mach_port_t object_name;
   mach_msg_type_number_t count;
   vm_address_t firstRegionBegin;
   vm_address_t lastRegionEnd;
   vm_size_t fullSize;
   count = VM_REGION_BASIC_INFO_COUNT_64;
   mach_vm_address_t address = 1;
   int regionCount = 0;
   int flag = 0;
   while (flag == 0)
   {
      //Attempts to get the region info for given task
      kret = mach_vm_region(task, &address, &size, VM_REGION_BASIC_INFO, (vm_region_info_t) &info, &count, &object_name);
      if (kret == KERN_SUCCESS)
      {
         if (regionCount == 0)
         {
            firstRegionBegin = address;
            regionCount += 1;
         }
         fullSize += size;
         address += size;
      }
      else
         flag = 1;
   }
   lastRegionEnd = address;
   printf("Base Address: %p\n",(void *) (uintptr_t)firstRegionBegin);
   printf("lastRegionEnd: %lu\n",lastRegionEnd);
   printf("fullSize: %lu\n",fullSize);

   return 0;
}
