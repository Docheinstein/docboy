#include "debuggablecore.h"

DebuggableCore::DebuggableCore(GameBoy &gameboy) :
    Core(gameboy), observer() {

}

void DebuggableCore::setObserver(IDebuggableCore::Observer *obs) {
    observer = obs;
}

void DebuggableCore::unsetObserver() {
    observer = nullptr;
}

//IDebuggableBus &DebuggableCore::getBus() {
//    return *gameboy.bus;
//}

IDebuggableCPU &DebuggableCore::getCpu() {
    return *gameboy.cpu;
}

void DebuggableCore::tick() {
    if (observer) {
        if (!observer->onTick())
            stop();
    }
    Core::tick();
}