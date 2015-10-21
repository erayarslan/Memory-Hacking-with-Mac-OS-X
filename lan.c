#import <Foundation/Foundation.h>
#include </usr/include/mach-o/getsect.h>
#include <stdio.h>
#include </usr/include/mach-o/dyld.h>
#include <string.h>

uint64_t StaticBaseAddress(void)
{
    const struct segment_command_64* command = getsegbyname("__TEXT");
    uint64_t addr = command->vmaddr;
    return addr;
}

intptr_t ImageSlide(void)
{
    char path[1024];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) != 0) return -1;
    for (uint32_t i = 0; i < _dyld_image_count(); i++)
    {
        if (strcmp(_dyld_get_image_name(i), path) == 0)
            return _dyld_get_image_vmaddr_slide(i);
    }
    return 0;
}

uint64_t DynamicBaseAddress(void)
{
    return StaticBaseAddress() + ImageSlide();
}

int main (int argc, const char *argv[])
{
    printf("dynamic base address (%0llx) = static base address (%0llx) + image slide (%0lx)\n", DynamicBaseAddress(), StaticBaseAddress(), ImageSlide());
    while (1) {}; // you can attach to this process via gdb/lldb to view the base address now :)
    return 0;
}
