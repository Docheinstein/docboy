#include "screens/graphicsoptionsscreen.h"

#include "controllers/navcontroller.h"
#include "controllers/uicontroller.h"

#include "components/menu/items.h"
#include "docboy/core/core.h"

GraphicsOptionsScreen::GraphicsOptionsScreen(Context context) :
    MenuScreen(context) {

    items.scaling = &menu.add_item(Spinner {[this] {
                                                on_decrease_scaling();
                                            },
                                            [this] {
                                                on_increase_scaling();
                                            }});

    items.palette = &menu.add_item(Spinner {[this] {
                                                on_prev_palette();
                                            },
                                            [this] {
                                                on_next_palette();
                                            }});
    menu.add_item(Button {"Back", [this] {
                              nav.pop();
                          }});
}

void GraphicsOptionsScreen::redraw() {
    items.scaling->text = "Scaling  " + std::to_string(ui.get_scaling());
    items.palette->text = "Palette  " + ui.get_current_palette().name;
    MenuScreen::redraw();
}

void GraphicsOptionsScreen::on_increase_scaling() {
    ui.set_scaling(ui.get_scaling() + 1);
    redraw();
}

void GraphicsOptionsScreen::on_decrease_scaling() {
    ui.set_scaling(std::max(ui.get_scaling() - 1, 1U));
    redraw();
}

void GraphicsOptionsScreen::on_prev_palette() {
    const auto num_palette = ui.get_palettes().size();
    const auto& current_palette = ui.get_current_palette();
    ui.set_current_palette((current_palette.index + num_palette - 1) % num_palette);
    redraw();
}

void GraphicsOptionsScreen::on_next_palette() {
    const auto num_palette = ui.get_palettes().size();
    const auto& current_palette = ui.get_current_palette();
    ui.set_current_palette((current_palette.index + 1) % num_palette);
    redraw();
}