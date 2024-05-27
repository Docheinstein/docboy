#ifndef MENUSCREEN_H
#define MENUSCREEN_H

#include "components/menu.h"
#include "screen.h"

struct SDL_Texture;

class MenuScreen : public Screen {
public:
    explicit MenuScreen(Context context);

    void redraw() override;
    void render() override;
    void handleEvent(const SDL_Event& event) override;

protected:
    SDL_Texture* menuBackgroundTexture {};
    SDL_Texture* menuForegroundTexture {};

    Menu menu;
};

#endif // MENUSCREEN_H
