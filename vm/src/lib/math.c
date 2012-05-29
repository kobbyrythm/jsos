#include <stddef.h>
#include <math.h>
#include <limits.h>
#include "lib.h"

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

static VAL Math_floor(js_vm_t* vm, void* state, VAL this, uint32_t argc, VAL* argv)
{
    double d = js_value_get_double(js_to_number(argc ? argv[0] : js_value_undefined()));
    return js_value_make_double(floor(d));
}

static VAL Math_round(js_vm_t* vm, void* state, VAL this, uint32_t argc, VAL* argv)
{
    double d = js_value_get_double(js_to_number(argc ? argv[0] : js_value_undefined()));
    return js_value_make_double(floor(d + 0.5));
}

static VAL Math_sin(js_vm_t* vm, void* state, VAL this, uint32_t argc, VAL* argv)
{
    double d = js_value_get_double(js_to_number(argc ? argv[0] : js_value_undefined()));
    return js_value_make_double(sin(d));
}

static VAL Math_cos(js_vm_t* vm, void* state, VAL this, uint32_t argc, VAL* argv)
{
    double d = js_value_get_double(js_to_number(argc ? argv[0] : js_value_undefined()));
    return js_value_make_double(cos(d));
}

static VAL Math_tan(js_vm_t* vm, void* state, VAL this, uint32_t argc, VAL* argv)
{
    double d = js_value_get_double(js_to_number(argc ? argv[0] : js_value_undefined()));
    return js_value_make_double(tan(d));
}

static VAL Math_min(js_vm_t* vm, void* state, VAL this, uint32_t argc, VAL* argv)
{
    double min = INFINITY;
    uint32_t i;
    for(i = 0; i < argc; i++) {
        double x = js_value_get_double(js_to_number(argv[i]));
        if(x < min) {
            min = x;
        }
    }
    return js_value_make_double(min);
}

static VAL Math_max(js_vm_t* vm, void* state, VAL this, uint32_t argc, VAL* argv)
{
    double max = -INFINITY;
    uint32_t i;
    for(i = 0; i < argc; i++) {
        double x = js_value_get_double(js_to_number(argv[i]));
        if(x > max) {
            max = x;
        }
    }
    return js_value_make_double(max);
}

static VAL Math_sqrt(js_vm_t* vm, void* state, VAL this, uint32_t argc, VAL* argv)
{
    double d = js_value_get_double(js_to_number(argc ? argv[0] : js_value_undefined()));
    return js_value_make_double(sqrt(d));
}

static VAL Math_pow(js_vm_t* vm, void* state, VAL this, uint32_t argc, VAL* argv)
{
    double a = js_value_get_double(js_to_number(argc >= 1 ? argv[0] : js_value_undefined()));
    double b = js_value_get_double(js_to_number(argc >= 2 ? argv[1] : js_value_undefined()));
    return js_value_make_double(pow(a, b));
}

static VAL Math_abs(js_vm_t* vm, void* state, VAL this, uint32_t argc, VAL* argv)
{
    double d = js_value_get_double(js_to_number(argc ? argv[0] : js_value_undefined()));
    return js_value_make_double(fabs(d));
}


static int mt[624];
static int mt_index;

void js_lib_math_seed_random(int seed)
{
    mt[0] = seed;
    uint32_t i;
    for(i = 1; i <= 623; i++) {
        mt[i] = 1812433253 * (mt[i-1] ^ ((mt[i-1]) >> 30)) + i;
    }
}

static void mt_generate()
{
    uint32_t i;
    for(i = 0; i < 624; i++) {
        int y = (mt[i] >> 31) + (mt[(i + 1) % 624] & 0x7fffffff);
        mt[i] = mt[(i + 397) % 624] ^ (y >> 1);
        if((y % 2) != 0) {
            mt[i] ^= 2567483615u;
        }
    }
}

static int mt_extract()
{
    if(mt_index == 0) {
        mt_generate();
    }
    int y = mt[mt_index];
    y ^= y >> 1;
    y ^= (y << 7) & 2636928640u;
    y ^= (y << 15) & 4022730752u;
    y ^= y >> 18;
    mt_index = (mt_index + 1) % 624;
    return y;
}

static VAL Math_random(js_vm_t* vm, void* state, VAL this, uint32_t argc, VAL* argv)
{
    int r = mt_extract();
    return js_value_make_double((double)r / (double)INT_MAX);
}

void js_lib_math_initialize(js_vm_t* vm)
{
    VAL Math = js_make_object(vm);
    js_object_put(vm->global_scope->global_object, js_cstring("Math"), Math);
    js_object_put(Math, js_cstring("floor"), js_value_make_native_function(vm, NULL, js_cstring("floor"), Math_floor, NULL));
    js_object_put(Math, js_cstring("round"), js_value_make_native_function(vm, NULL, js_cstring("round"), Math_round, NULL));
    js_object_put(Math, js_cstring("cos"), js_value_make_native_function(vm, NULL, js_cstring("cos"), Math_cos, NULL));
    js_object_put(Math, js_cstring("sin"), js_value_make_native_function(vm, NULL, js_cstring("sin"), Math_sin, NULL));
    js_object_put(Math, js_cstring("tan"), js_value_make_native_function(vm, NULL, js_cstring("tan"), Math_tan, NULL));
    js_object_put(Math, js_cstring("min"), js_value_make_native_function(vm, NULL, js_cstring("min"), Math_min, NULL));
    js_object_put(Math, js_cstring("max"), js_value_make_native_function(vm, NULL, js_cstring("max"), Math_max, NULL));
    js_object_put(Math, js_cstring("sqrt"), js_value_make_native_function(vm, NULL, js_cstring("sqrt"), Math_sqrt, NULL));
    js_object_put(Math, js_cstring("pow"), js_value_make_native_function(vm, NULL, js_cstring("pow"), Math_pow, NULL));
    js_object_put(Math, js_cstring("abs"), js_value_make_native_function(vm, NULL, js_cstring("abs"), Math_abs, NULL));
    js_object_put(Math, js_cstring("random"), js_value_make_native_function(vm, NULL, js_cstring("random"), Math_random, NULL));
    js_object_put(Math, js_cstring("PI"), js_value_make_double(M_PI));
}