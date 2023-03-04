#include "gpu.h"

GPU::GPU(IDisplay &display, VRAM &vram, IO &io) :
    display(display), vram(vram), io(io), pixels() {

}

void GPU::tick() {
    display.render(pixels);
}
