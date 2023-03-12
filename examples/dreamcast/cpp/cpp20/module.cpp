// helloworld.cpp
export module TestModule;  // module declaration
#include <stdio.h>
 
export int module_variable = 0;

export void verifyModule_(int value) {
    module_variable = value;
    printf("[Module] Setting exported public value: %d\n", module_variable);
}