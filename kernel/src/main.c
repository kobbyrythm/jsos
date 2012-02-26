#include <gc.h>
#include <vm.h>
#include <value.h>
#include <exception.h>
#include <stdarg.h>
#include "multiboot.h"
#include "mm.h"
#include "console.h"
#include "panic.h"
#include "gdt.h"
#include "interrupt.h"
#include "io.h"
#include "lib.h"

static VAL console_log(js_vm_t* vm, void* state, VAL this, uint32_t argc, VAL* argv)
{
    uint32_t i;
    for(i = 0; i < argc; i++) {
       kprintf("%s%s", i ? " " : "", js_value_get_pointer(js_to_string(argv[i]))->string.buff);
    }
    kprintf("\n");
    return js_value_undefined();
}

static VAL Kernel_load_image(js_vm_t* vm, void* state, VAL this, uint32_t argc, VAL* argv)
{
    if(argc == 0 || js_value_get_type(argv[0]) != JS_T_STRING) {
        js_throw_message(vm, "Kernel.loadImage() expects string");
    }
    js_string_t* str = &js_value_get_pointer(argv[0])->string;
    js_image_t* image = js_image_parse(str->buff, str->length);
    if(!image) {
        js_throw_message(vm, "Couldn't parse image");
    }
    js_vm_exec(vm, image, 0, vm->global_scope, js_value_null(), 0, NULL);
    return js_value_true();
}

static VAL Kernel_memory_usage(js_vm_t* vm, void* state, VAL this, uint32_t argc, VAL* argv)
{
    return js_value_make_double(js_gc_memory_usage());
}

static VAL Kernel_run_gc(js_vm_t* vm, void* state, VAL this, uint32_t argc, VAL* argv)
{
    js_gc_run();
    return js_value_undefined();
}

void load_modules(VAL object, multiboot_module_t* modules, uint32_t count)
{
    kprintf("Loading %d modules:\n", count);
    uint32_t i;
    for(i = 0; i < count; i++) {
        kprintf("  %s\n", modules[i].cmdline);
        js_string_t* name = js_cstring((char*)modules[i].cmdline);
        uint32_t size = modules[i].mod_end - modules[i].mod_start;
        VAL data = js_value_make_string((char*)modules[i].mod_start, size);
        js_object_put(object, name, data);
    }
}

static js_vm_t* vm;

void kmain_(struct multiboot_info* mbd, uint32_t magic)
{
    console_clear();
    
    if(magic != MULTIBOOT_BOOTLOADER_MAGIC)         panic("multiboot did not pass correct magic number");
    if(!(mbd->flags & MULTIBOOT_INFO_MEMORY))       panic("multiboot did not pass memory information");
    if(!(mbd->flags & MULTIBOOT_INFO_MEM_MAP))      panic("multiboot did not pass memory map");
    
    kprintf("JSOS\n");
    mm_init((multiboot_memory_map_t*)mbd->mmap_addr, mbd->mmap_length);
    gdt_init();
    
    vm = js_vm_new();
    js_gc_register_global(&vm, sizeof(vm));
    js_set_panic_handler(js_panic_handler);
    
    VAL console = js_make_object(vm);
    js_object_put(console, js_cstring("log"), js_value_make_native_function(vm, NULL, js_cstring("log"), console_log, NULL));
    js_object_put(vm->global_scope->global_object, js_cstring("console"), console);
    
    VAL Kernel = js_make_object(vm);
    js_object_put(vm->global_scope->global_object, js_cstring("Kernel"), Kernel);
    js_object_put(Kernel, js_cstring("loadImage"), js_value_make_native_function(vm, NULL, js_cstring("loadImage"), Kernel_load_image, NULL));
    js_object_put(Kernel, js_cstring("memoryUsage"), js_value_make_native_function(vm, NULL, js_cstring("memoryUsage"), Kernel_memory_usage, NULL));
    js_object_put(Kernel, js_cstring("runGC"), js_value_make_native_function(vm, NULL, js_cstring("runGC"), Kernel_run_gc, NULL));
    
    cli();
    idt_init(vm);
    sti();
    
    io_init(vm);
    lib_binary_utils_init(vm);
    
    VAL modules = js_make_object(vm);
    js_object_put(Kernel, js_cstring("modules"), modules);
    load_modules(modules, (multiboot_module_t*)mbd->mods_addr, mbd->mods_count);
    
    VAL vinit = js_object_get(modules, js_cstring("/kernel/init.jmg"));
    if(js_value_get_type(vinit) != JS_T_STRING) {
        panic("could not load /kernel/init.jmg");
    }
    js_value_t* sinit = js_value_get_pointer(vinit);
    js_image_t* image = js_image_parse(sinit->string.buff, sinit->string.length);
    
    kprintf("Handing over control to JavaScript:\n\n");
    
    VAL exception;
    JS_TRY({
        js_vm_exec(vm, image, 0, vm->global_scope, js_value_null(), 0, NULL);
    }, exception, {
        panicf("Unhandled exception: %s", js_value_get_pointer(js_to_string(exception))->string.buff);
    });
}

void kmain(struct multiboot_info* mbd, uint32_t magic)
{
    int dummy;
    js_gc_init(&dummy);
    kmain_(mbd, magic);
    for(;;) {
        interrupt_dispatch_events();
        __asm__ volatile("hlt");
    }
}

