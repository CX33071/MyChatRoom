#include <signal.h>

class IgnoreSigPipe
{
public:
    IgnoreSigPipe()
    {
        ::signal(SIGPIPE, SIG_IGN); // 	忽略 SIGPIPE，防止写关闭 socket 时进程被杀
    }
};
