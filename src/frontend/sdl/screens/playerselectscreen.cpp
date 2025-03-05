#include "screens/playerselectscreen.h"

#include "components/menu/items.h"

PlayerSelectScreen::PlayerSelectScreen(Screen::Context ctx, std::function<void()>&& p1_callback,
                                       std::function<void()>&& p2_callback) :
    MenuScreen(ctx),
    player1_callback {std::move(p1_callback)},
    player2_callback {std::move(p2_callback)} {

    menu.add_item(Button {"Player 1", [this] {
                              ASSERT(player1_callback);
                              player1_callback();
                          }});
    menu.add_item(Button {"Player 2", [this] {
                              ASSERT(player2_callback);
                              player2_callback();
                          }});
}
