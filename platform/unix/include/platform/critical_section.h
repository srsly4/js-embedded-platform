#ifndef JS_EMBEDDED_PLATFORM_CRITICAL_SECTION_H
#define JS_EMBEDDED_PLATFORM_CRITICAL_SECTION_H

#include <pthread.h>

#define CRITICAL_SECTION_ENTER() _enter_critical_section()
#define CRITICAL_SECTION_EXIT() _exit_critical_section()


void _enter_critical_section();
void _exit_critical_section();


#endif //JS_EMBEDDED_PLATFORM_CRITICAL_SECTION_H
