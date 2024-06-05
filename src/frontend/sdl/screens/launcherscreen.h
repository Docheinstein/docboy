#ifndef LAUNCHERSCREEN_H
#define LAUNCHERSCREEN_H

#include "screens/screen.h"

struct SDL_Texture;

class LauncherScreen : public Screen {
public:
    explicit LauncherScreen(Context context);

    void redraw() override;
    void render() override;
    void handle_event(const SDL_Event& event) override;

private:
    SDL_Texture* background_texture {};
    SDL_Texture* foreground_texture {};
};

#endif // LAUNCHERSCREEN_H
