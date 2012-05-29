#include <stdlib.h>
#include "scope.h"
#include "vm.h"
#include "gc.h"
#include "exception.h"

js_scope_t* js_scope_make_global(js_vm_t* vm, VAL object)
{
    js_scope_t* scope = js_alloc(sizeof(js_scope_t));
    scope->parent = NULL;
    scope->global = scope;
    scope->vm = vm;
    if(js_value_is_primitive(object)) {
        js_panic("primitive passed as global object");
    }
    scope->global_object = object;
    return scope;
}

VAL js_scope_get_var(js_scope_t* scope, uint32_t index, uint32_t upper_scopes)
{
    if(upper_scopes > 0) {
        return scope->parent ? js_scope_get_var(scope->parent, index, upper_scopes - 1) : js_value_undefined();
    }
    if(index >= scope->locals.count) {
        return js_value_undefined();
    }
    return scope->locals.vars[index];
}

void js_scope_set_var(js_scope_t* scope, uint32_t index, uint32_t upper_scopes, VAL value)
{
    uint32_t old_size;
    if(upper_scopes > 0) {
        if(scope->parent) {
            js_scope_set_var(scope->parent, index, upper_scopes - 1, value);
        }
        return;
    }
    old_size = scope->locals.count;
    if(old_size <= index) {
        while(scope->locals.count <= index) {
            scope->locals.count *= 2;
        }    
        scope->locals.vars = js_realloc(scope->locals.vars, scope->locals.count * sizeof(VAL));
        for(; old_size < scope->locals.count; old_size++) {
            scope->locals.vars[old_size] = js_value_undefined();
        }
    }
    scope->locals.vars[index] = value;
}

js_scope_t* js_scope_close(js_scope_t* scope, VAL callee)
{
    return js_scope_close_placement(js_alloc(sizeof(js_scope_t)), scope, callee, 4, js_alloc(4 * sizeof(VAL)));
}

js_scope_t* js_scope_close_placement(js_scope_t* new_scope, js_scope_t* scope, VAL callee, uint32_t var_count, VAL* vars)
{
    uint32_t i;
    new_scope->vm = scope->vm;
    new_scope->parent = scope;
    new_scope->global = scope->global;
    new_scope->locals.callee = callee;
    new_scope->locals.count = var_count;
    new_scope->locals.vars = vars;
    for(i = 0; i < var_count; i++) {
        new_scope->locals.vars[i] = js_value_undefined();
    }
    return new_scope;
}

VAL js_scope_get_global_var(js_scope_t* scope, js_string_t* name)
{
    if(js_object_has_property(scope->global->global_object, name)) {
        return js_object_get(scope->global->global_object, name);
    } else {
        js_throw_error(scope->vm->lib.ReferenceError, "undefined variable %s", name->buff);
    }
}

void js_scope_set_global_var(js_scope_t* scope, js_string_t* name, VAL value)
{
    js_object_put(scope->global->global_object, name, value);
}

bool js_scope_global_var_exists(js_scope_t* scope, js_string_t* name)
{
    return js_object_has_property(scope->global->global_object, name);
}

void js_scope_delete_global_var(js_scope_t* scope, js_string_t* name)
{
    (void)scope;
    (void)name;
    /*
    js_value_t* obj = js_value_get_pointer(scope->global->global_object);
    obj->object.vtable->delete(obj, name);
    */
    // @TODO
}