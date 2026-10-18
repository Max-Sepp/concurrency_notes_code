#ifndef SPINLOCK_H
#define SPINLOCK_H

class Spinlock {
  public:
    virtual ~Spinlock() = default;

    virtual void Lock() = 0;
    virtual void Unlock() = 0;
};

#endif // SPINLOCK_H
