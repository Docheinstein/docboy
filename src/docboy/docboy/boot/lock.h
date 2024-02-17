#ifndef BOOTLOCK_H
#define BOOTLOCK_H

class Mmu;

class BootLock {
public:
    explicit BootLock(Mmu& mmu);

    void lock();

private:
    Mmu& mmu;
};

#endif // BOOTLOCK_H