#ifndef LOCK_H
#define LOCK_H

class ILock {
  public:
    virtual ~ILock() = default;

    virtual void Lock() = 0;
    virtual void Unlock() = 0;
};

#endif // LOCK_H
