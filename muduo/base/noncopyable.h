#ifndef NONCOPYABLE_H
#define NONCOPYABLE_H
namespace mulib
{
    class noncopyable{
    public:
        noncopyable(const noncopyable &) = delete;
        noncopyable &operator=(const noncopyable &) = delete;

    protected:
        noncopyable() = default;
        ~noncopyable() = default;
    };
}
#endif 
