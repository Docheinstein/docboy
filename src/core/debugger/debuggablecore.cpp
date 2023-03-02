#include "debuggablecore.h"

DebuggableCore::DebuggableCore() : observer() {

}

void DebuggableCore::setObserver(IDebuggableCore::Observer *obs) {
    observer = obs;
}

void DebuggableCore::unsetObserver() {
    observer = nullptr;
}

IDebuggableBus &DebuggableCore::getBus() {
    return gameboy.bus;
}

IDebuggableCpu &DebuggableCore::getCpu() {
    return gameboy.cpu;
}

void DebuggableCore::tick() {
    if (observer) {
        if (!observer->onTick())
            stop();
    }
    Core::tick();
}