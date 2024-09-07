#include <QtGlobal>

#ifdef __APPLE__

#include <unistd.h>
#include <sys/mman.h>
#include <pthread.h>
#include <libkern/OSCacheControl.h>
#include "virtual_asm.h"

// @README https://developer.apple.com/documentation/apple-silicon/porting-just-in-time-compilers-to-apple-silicon

namespace assembler {
#ifdef Q_PROCESSOR_X86_64 // Pre-2020 Intel Macs and Hackintosh.
unsigned Processor::maxIntArgs64() { return 6; }

unsigned Processor::maxFloatArgs64() { return 8; }

bool Processor::isIntArg64Register(unsigned char number, unsigned char arg) { return number < 6; }

bool Processor::isFloatArg64Register(unsigned char number, unsigned char arg) { return number < 8; }
Register Processor::intArg64(unsigned char number, unsigned char arg) {
    switch(number) {
        case 0: return Register(*this, EDI);
        case 1: return Register(*this, ESI);
        case 2: return Register(*this, EDX);
        case 3: return Register(*this, ECX);
        case 4: return Register(*this, R8);
        case 5: return Register(*this, R9);
        default: throw "Integer64 argument index out of bounds";
    }
}

Register Processor::floatArg64(unsigned char number, unsigned char arg) {
    switch(number) {
        case 0: return Register(*this, XMM0);
        case 1: return Register(*this, XMM1);
        case 2: return Register(*this, XMM2);
        case 3: return Register(*this, XMM3);
        case 4: return Register(*this, XMM4);
        case 5: return Register(*this, XMM5);
        case 6: return Register(*this, XMM6);
        case 7: return Register(*this, XMM7);
        default: throw "Float64 argument index out of bounds";
    }
}

#else // Q_PROCESSOR_APPLE_SILICON - M-series CPU

unsigned Processor::maxIntArgs64() { return 8; }

unsigned Processor::maxFloatArgs64() { return 8; }

bool Processor::isIntArg64Register(unsigned char number, unsigned char arg) { return number < 8; }

bool Processor::isFloatArg64Register(unsigned char number, unsigned char arg) { return number < 8; }

Register Processor::intArg64(unsigned char number, unsigned char arg) {
    switch(number) {
        case 0: return Register(*this, X0);
        case 1: return Register(*this, X1);
        case 2: return Register(*this, X2);
        case 3: return Register(*this, X3);
        case 4: return Register(*this, X4);
        case 5: return Register(*this, X5);
        case 6: return Register(*this, X6);
        case 7: return Register(*this, X7);
        default: throw "Integer64 argument index out of bounds";
    }
}

Register Processor::floatArg64(unsigned char number, unsigned char arg) {
    switch(number) {
        case 0: return Register(*this, D0);
        case 1: return Register(*this, D1);
        case 2: return Register(*this, D2);
        case 3: return Register(*this, D3);
        case 4: return Register(*this, D4);
        case 5: return Register(*this, D5);
        case 6: return Register(*this, D6);
        case 7: return Register(*this, D7);
        default: throw "Float64 argument index out of bounds";
    }
}
#endif

Register Processor::intArg64(unsigned char number, unsigned char arg, Register defaultReg) {
    if(isIntArg64Register(number, arg)) return intArg64(number, arg);
    return defaultReg;
}

Register Processor::floatArg64(unsigned char number, unsigned char arg, Register defaultReg) {
    if(isFloatArg64Register(number, arg)) return floatArg64(number, arg);
    return defaultReg;
}

#ifdef Q_PROCESSOR_X86_64
Register Processor::intReturn64() { return Register(*this, EAX); }
Register Processor::floatReturn64() { return Register(*this, XMM0); }
#else // Q_PROCESSOR_AARCH64
Register Processor::intReturn64() { return Register(*this, X0); }
Register Processor::floatReturn64() { return Register(*this, D0); }
#endif

CodePage::CodePage(unsigned int Size, void* requestedStart) : used(0), references(1), final(false) {
    unsigned minPageSize = getMinimumPageSize();
    unsigned pages = Size / minPageSize;
    size = Size;

    if(size % minPageSize != 0) pages += 1; // Add page if not enough

    size_t reqptr = (size_t)requestedStart;
    if(reqptr % minPageSize != 0) reqptr -= (reqptr % minPageSize); // Kind of .align

    page = mmap(
        (void*)reqptr, //Not sure about allocating page near reqptr, probably NULL will be good
        size,
        PROT_READ | PROT_WRITE | PROT_EXEC,
        MAP_ANONYMOUS | MAP_PRIVATE | MAP_JIT,
        -1,
        0);
    
    if(MAP_FAILED == page){
        printf("Something went wrong with mmap() at CodePage(JIT) alloc: %s\n", strerror(errno));
        exit(1);
    }
    pthread_jit_write_protect_np(0); // -rw-r--r--
    sys_icache_invalidate(page, size);
}

void CodePage::grab() { ++references; }

void CodePage::drop() { if(--references == 0) delete this; }

CodePage::~CodePage() { munmap(page, size); }

void CodePage::finalize() {
    pthread_jit_write_protect_np(1); // -r-x-r--r--
    sys_icache_invalidate(page, size);
    final = true;
}

unsigned int CodePage::getMinimumPageSize() { return getpagesize(); }

void CriticalSection::enter() { pthread_mutex_lock((pthread_mutex_t*)pLock); }

void CriticalSection::leave() { pthread_mutex_unlock((pthread_mutex_t*)pLock); }

CriticalSection::CriticalSection() {
    pthread_mutex_t* mutex = new pthread_mutex_t();
    pthread_mutex_init(mutex, 0);

    pLock = mutex;
}
CriticalSection::~CriticalSection() {
    pthread_mutex_t* mutex = (pthread_mutex_t*)pLock;
    pthread_mutex_destroy(mutex);
    delete mutex;
}

};
#endif

#ifdef Q_PROCESSOR_POWER
#error "G-series CPU is not supported."
#endif