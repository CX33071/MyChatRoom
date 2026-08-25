#ifndef MUDUO_BASE_CURRENTTHREAD_H
#define MUDUO_BASE_CURRENTTHREAD_H
#include <sys/types.h>
namespace mulib{
    namespace CurrentThread{
        extern __thread int t_cachedTid;
        pid_t tid();
        pid_t gettid();
        
    }
}
#endif
