#ifndef UTILS_H_
#define UTILS_H_
#include "config.h"
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <arpa/inet.h>

#define STR(x) #x
#define TOSTR(x) STR(x)

#define LOGI(format, ...) do {\
                          time_t now = time(NULL);\
                          char timestr[20];\
                          strftime(timestr, 20, TIME_FORMAT, localtime(&now));\
                          fprintf(stderr, "\e[01;32m %s INFO: \e[0m" format "\n", timestr, ##__VA_ARGS__);}\
                          while(0)
#define LOGE(format, ...) do {\
                          time_t now = time(NULL);\
                          char timestr[20];\
                          strftime(timestr, 20, TIME_FORMAT, localtime(&now));\
                          fprintf(stderr, "\e[01;35m %s ERROR: \e[0m" format " on File: %s Line: %s\n", timestr, ##__VA_ARGS__, __FILE__, TOSTR(__LINE__));}\
                          while(0)
#define FATAL(format, ...) do {\
                          time_t now = time(NULL);\
                          char timestr[20];\
                          strftime(timestr, 20, TIME_FORMAT, localtime(&now));\
                          fprintf(stderr, "\e[01;31m %s FATAL: \e[0m" format " on File: %s Line: %s\n", timestr, ##__VA_ARGS__, __FILE__, TOSTR(__LINE__));exit(1);}\
                          while(0)

unsigned long long int byte_swap_64(unsigned long long int x);

#endif /* !UTILS_H_ */

