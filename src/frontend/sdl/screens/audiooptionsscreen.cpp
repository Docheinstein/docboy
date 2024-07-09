#include "screens/audiooptionsscreen.h"

#include "controllers/corecontroller.h"
#include "controllers/maincontroller.h"

#include "SDL3/SDL.h"

AudioOptionsScreen::AudioOptionsScreen(Context context) :
    MenuScreen {context} {

    items.audio_enabled = &menu.add_item(MenuItem {"Audio"}.on_change([this] {
        on_change_audio_enabled();
    }));

    items.volume = &menu.add_item(MenuItem {"Volume"}
                                      .on_prev([this] {
                                          on_decrease_volume();
                                      })
                                      .on_next([this] {
                                          on_increase_volume();
                                      }));
}

void AudioOptionsScreen::redraw() {
    items.audio_enabled->text = "Audio   " + std::string {main.is_audio_enabled() ? "Enabled" : "Disabled"};
    items.volume->text = "Volume  " + std::to_string(main.get_volume());
    MenuScreen::redraw();
}

void AudioOptionsScreen::on_change_audio_enabled() {
    main.set_audio_enabled(!main.is_audio_enabled());
    redraw();
}

void AudioOptionsScreen::on_decrease_volume() {
    int32_t volume = std::max(main.get_volume() - 10, 0);
    ASSERT(volume >= 0 && volume <= MainController::VOLUME_MAX);
    main.set_volume(volume);
    redraw();
}

void AudioOptionsScreen::on_increase_volume() {
    int32_t volume = std::min(main.get_volume() + 10, static_cast<int>(MainController::VOLUME_MAX));
    ASSERT(volume >= 0 && volume <= MainController::VOLUME_MAX);
    main.set_volume(volume);
    redraw();
}
