#ifndef LAUNCHERSCREEN_H
#define LAUNCHERSCREEN_H

#include "screen.h"

struct SDL_Texture;

class LauncherScreen : public Screen {
public:
    explicit LauncherScreen(Context context);

    void redraw() override;
    void render() override;
    void handleEvent(const SDL_Event& event) override;

private:
    SDL_Texture* backgroundTexture {};
    SDL_Texture* foregroundTexture {};
};

#endif // LAUNCHERSCREEN_H
