#include "graphicsoptionsscreen.h"
#include "controllers/navcontroller.h"
#include "controllers/uicontroller.h"
#include "docboy/core/core.h"

GraphicsOptionsScreen::GraphicsOptionsScreen(Context context) :
    MenuScreen(context) {

    scalingItem = &menu.addItem({"Scaling", nullptr,
                                 [this] {
                                     onDecreaseScaling();
                                 },
                                 [this] {
                                     onIncreaseScaling();
                                 }});
    paletteItem = &menu.addItem({"Palette", nullptr,
                                 [this] {
                                     onPrevPalette();
                                 },
                                 [this] {
                                     onNextPalette();
                                 }});
    menu.addItem({"Back", [this] {
                      nav.pop();
                  }});
}

void GraphicsOptionsScreen::redraw() {
    scalingItem->text = "Scaling  " + std::to_string(ui.getScaling());
    paletteItem->text = "Palette  " + ui.getCurrentPalette().name;
    MenuScreen::redraw();
}

void GraphicsOptionsScreen::onIncreaseScaling() {
    ui.setScaling(ui.getScaling() + 1);
    redraw();
}

void GraphicsOptionsScreen::onDecreaseScaling() {
    ui.setScaling(std::max(ui.getScaling() - 1, 1U));
    redraw();
}

void GraphicsOptionsScreen::onPrevPalette() {
    const auto numPalette = ui.getPalettes().size();
    const auto& currentPalette = ui.getCurrentPalette();
    ui.setCurrentPalette((currentPalette.index + numPalette - 1) % numPalette);
    redraw();
}

void GraphicsOptionsScreen::onNextPalette() {
    const auto numPalette = ui.getPalettes().size();
    const auto& currentPalette = ui.getCurrentPalette();
    ui.setCurrentPalette((currentPalette.index + 1) % numPalette);
    redraw();
}