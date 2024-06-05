#ifndef MENUSCREEN_H
#define MENUSCREEN_H

#include "components/menu.h"
#include "screens/screen.h"

struct SDL_Texture;

class MenuScreen : public Screen {
public:
    explicit MenuScreen(Context context);

    void redraw() override;
    void render() override;
    void handle_event(const SDL_Event& event) override;

protected:
    SDL_Texture* menu_background_texture {};
    SDL_Texture* menu_foreground_texture {};

    Menu menu;
};

#endif // MENUSCREEN_H
