#pragma once


#ifdef __cplusplus
extern "C" {
#endif
int test();

typedef void* Fun;
Fun MkFun(char*);

#ifdef __cplusplus
}
#endif