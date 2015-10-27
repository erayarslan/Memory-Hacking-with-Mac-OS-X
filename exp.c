// clang fixexpose.c -framework Foundation -o fixexpose

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <string.h>
#include <assert.h>
#include <errno.h>
#include <grp.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/sysctl.h>

#include <mach/mach.h>
#include <mach/mach_vm.h>

#include <CoreServices/CoreServices.h>
char* getBundleVersion(const char* bundlePath) {
    CFURLRef bundlePackageURL = NULL;
    bundlePackageURL = CFURLCreateFromFileSystemRepresentation(
        kCFAllocatorDefault,
        (const UInt8*)bundlePath,
        strlen(bundlePath),
        true);

    assert(bundlePackageURL);

    CFBundleRef bundle = NULL;
    bundle = CFBundleCreate(kCFAllocatorDefault, bundlePackageURL);
    assert(bundle);

    CFStringRef versionStr = (CFStringRef)CFBundleGetValueForInfoDictionaryKey(
        bundle, kCFBundleVersionKey);

    int length = CFStringGetLength(versionStr)+1;
    char* ret = (char*)malloc(length);

    Boolean v = CFStringGetCString(versionStr, ret, length, kCFStringEncodingASCII);
    assert(v);

    CFRelease(bundle);
    CFRelease(bundlePackageURL);

    return ret;
}

//from http://developer.apple.com/mac/library/qa/qa2001/qa1123.html
typedef struct kinfo_proc kinfo_proc;
static int getBSDProcessList(kinfo_proc **procList, size_t *procCount)
    // Returns a list of all BSD processes on the system.  This routine
    // allocates the list and puts it in *procList and a count of the
    // number of entries in *procCount.  You are responsible for freeing
    // this list (use "free" from System framework).
    // On success, the function returns 0.
    // On error, the function returns a BSD errno value.
{
    int                 err;
    kinfo_proc *        result;
    bool                done;
    static const int    name[] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0 };
    // Declaring name as const requires us to cast it when passing it to
    // sysctl because the prototype doesn't include the const modifier.
    size_t              length;

    assert( procList != NULL);
    assert(*procList == NULL);
    assert(procCount != NULL);

    *procCount = 0;

    // We start by calling sysctl with result == NULL and length == 0.
    // That will succeed, and set length to the appropriate length.
    // We then allocate a buffer of that size and call sysctl again
    // with that buffer.  If that succeeds, we're done.  If that fails
    // with ENOMEM, we have to throw away our buffer and loop.  Note
    // that the loop causes use to call sysctl with NULL again; this
    // is necessary because the ENOMEM failure case sets length to
    // the amount of data returned, not the amount of data that
    // could have been returned.

    result = NULL;
    done = false;
    do {
        assert(result == NULL);

        // Call sysctl with a NULL buffer.

        length = 0;
        err = sysctl( (int *) name, (sizeof(name) / sizeof(*name)) - 1,
                      NULL, &length,
                      NULL, 0);
        if (err == -1) {
            err = errno;
        }

        // Allocate an appropriately sized buffer based on the results
        // from the previous call.

        if (err == 0) {
            result = malloc(length);
            if (result == NULL) {
                err = ENOMEM;
            }
        }

        // Call sysctl again with the new buffer.  If we get an ENOMEM
        // error, toss away our buffer and start again.

        if (err == 0) {
            err = sysctl( (int *) name, (sizeof(name) / sizeof(*name)) - 1,
                          result, &length,
                          NULL, 0);
            if (err == -1) {
                err = errno;
            }
            if (err == 0) {
                done = true;
            } else if (err == ENOMEM) {
                assert(result != NULL);
                free(result);
                result = NULL;
                err = 0;
            }
        }
    } while (err == 0 && ! done);

    // Clean up and establish post conditions.

    if (err != 0 && result != NULL) {
        free(result);
        result = NULL;
    }
    *procList = result;
    if (err == 0) {
        *procCount = length / sizeof(kinfo_proc);
    }

    assert( (err == 0) == (*procList != NULL) );

    return err;
}

cpu_type_t getProcessArchitecture(pid_t pid) {
    int mib[CTL_MAXNAME];
    size_t mibLen = CTL_MAXNAME;
    int err = sysctlnametomib("sysctl.proc_cputype", mib, &mibLen);

    assert(err == 0);

    assert(mibLen < CTL_MAXNAME);
    mib[mibLen] = pid;
    mibLen += 1;

    cpu_type_t cpuType;
    size_t cpuTypeSize = sizeof(cpuType);
    err = sysctl(mib, mibLen, &cpuType, &cpuTypeSize, 0, 0);

    assert(err == 0);

    return cpuType;
}

mach_vm_address_t getTaskBaseAddress(mach_port_name_t taskPort) {
    mach_vm_address_t vmoffset = 0;
    mach_vm_size_t vmsize;
    uint32_t nesting_depth = 0;
    struct vm_region_submap_info_64 vbr;
    mach_msg_type_number_t vbrcount = VM_REGION_SUBMAP_INFO_COUNT_64;
    kern_return_t kr;

    //assume the first region is the task __text
    kr = mach_vm_region_recurse(taskPort, &vmoffset, &vmsize,
        &nesting_depth,
        (vm_region_recurse_info_t)&vbr,
        &vbrcount);
    assert(kr == KERN_SUCCESS);

    return vmoffset;
}

int main (int argc, char** argv) {
    // check if we have correct privileges
    struct group *procmodGroup = getgrnam("procmod");
    if (getuid() != 0 && getegid() != procmodGroup->gr_gid) {
        fprintf(stderr, "Must be run as root or with procmod\n");
        exit(1);
    }


    //find the info of the dock process
    kinfo_proc *procList = NULL;
    size_t procCount = 0;

    getBSDProcessList(&procList, &procCount);

    pid_t pid = -1;
    int i;
    for (i=0; i<procCount; i++) {
        if (strcmp(procList[i].kp_proc.p_comm, "Dock") == 0) {
            pid = procList[i].kp_proc.p_pid;
            break;
        }
    }
    assert(pid != -1);
    printf("Dock pid is %d\n", pid);

    cpu_type_t arch = getProcessArchitecture(pid);
    char* dockVersion = getBundleVersion("/System/Library/CoreServices/Dock.app");

    printf("Dock version is %s\n", dockVersion);


    mach_vm_address_t hackOffset;
    mach_vm_size_t hackSize;
    pointer_t hackData = 0;

    hackOffset = 0x593ed;
    hackSize = 1;
    hackData = (pointer_t)"\x06";

    /*
    if (arch == (CPU_TYPE_X86 | CPU_ARCH_ABI64)) {
        if (strcmp(dockVersion, "1040.10") == 0) {
            hackOffset = 0x593ed;
            hackSize = 1;
            hackData = (pointer_t)"\x06";
        } else if (strcmp(dockVersion, "1040.36") == 0) {
            hackOffset = 0x56afa;
            hackSize = 1;
            hackData = (pointer_t)"\x06";
        } else if (strcmp(dockVersion, "1168.8") == 0) {
            hackOffset = 0x4b8d1;
            hackSize = 2;
            hackData = (pointer_t)"\x90\xE9";
        }
    } else if (arch == CPU_TYPE_X86) {
        if (strcmp(dockVersion, "1040.10") == 0) {
            hackOffset = 0x583ab;
            hackSize = 1;
            hackData = (pointer_t)"\x06";
        }
    }
    */

    if (!hackData) {
        fprintf(stderr, "Dock version not supported\n");
        exit(1);
    }

    free(dockVersion);


    mach_port_name_t port;
    if(task_for_pid(mach_task_self(), pid, &port)) {
        fprintf(stderr, "Can't open Dock task\n");
        exit(1);
    }

    mach_vm_address_t baseAddress = getTaskBaseAddress(port);
    printf("Base address is 0x%llx\n", baseAddress);

    printf("Suspending Dock\n");
    if (task_suspend(port)) {
        fprintf(stderr, "Can't suspend Dock\n");
        exit(1);
    }

    mach_vm_address_t writeAddress = baseAddress + hackOffset;
    printf("Target address is 0x%llx\n", writeAddress);

    printf("Making 0x%llx writable\n", writeAddress);
    if (vm_protect(port, writeAddress, hackSize, 0, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE)) {
        fprintf(stderr, "Can't set Dock memory protection\n");
        task_resume(port);
        exit(1);
    }

    printf("Writing patch to 0x%llx\n", writeAddress);
    if(vm_write(port, writeAddress, hackData, hackSize)) {
        fprintf(stderr, "Can't write to Dock memory\n");
        task_resume(port);
        exit(1);
    }

    printf("Resuming Dock\n");
    if (task_resume(port)) {
        fprintf(stderr, "Can't resume Dock\n");
        exit(1);
    }

    printf("Done\n");


    return 0;
}
