#include "screens/graphicsoptionsscreen.h"

#include "controllers/navcontroller.h"
#include "controllers/uicontroller.h"

#include "components/menu/items.h"

GraphicsOptionsScreen::GraphicsOptionsScreen(Context context) :
    MenuScreen(context) {

    items.scaling = &menu.add_item(Spinner {[this] {
                                                on_decrease_scaling();
                                            },
                                            [this] {
                                                on_increase_scaling();
                                            }});

#ifndef ENABLE_CGB
    items.palette = &menu.add_item(Spinner {[this] {
                                                on_prev_palette();
                                            },
                                            [this] {
                                                on_next_palette();
                                            }});
#endif
    menu.add_item(Button {"Back", [this] {
                              nav.pop();
                          }});
}

void GraphicsOptionsScreen::redraw() {
    items.scaling->text = "Scaling  " + std::to_string(ui.get_scaling());
#ifndef ENABLE_CGB
    items.palette->text = "Palette  " + ui.get_current_appearance().name;
#endif
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

#ifndef ENABLE_CGB
void GraphicsOptionsScreen::on_prev_palette() {
    const auto num_appearances = ui.get_appearances().size();
    const auto& current_appearance = ui.get_current_appearance();
    ui.set_current_appearance((current_appearance.index + num_appearances - 1) % num_appearances);
    redraw();
}

void GraphicsOptionsScreen::on_next_palette() {
    const auto num_appearances = ui.get_appearances().size();
    const auto& current_appearance = ui.get_current_appearance();
    ui.set_current_appearance((current_appearance.index + 1) % num_appearances);
    redraw();
}
#endif