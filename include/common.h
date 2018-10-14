#ifndef PROJECT_COMMON_H
#define PROJECT_COMMON_H

void entrypoint(void);

void eventloop(void);

void set_user_code(char* code);

char* get_user_code();

void clear_eventloop(void);

#endif //PROJECT_COMMON_H
