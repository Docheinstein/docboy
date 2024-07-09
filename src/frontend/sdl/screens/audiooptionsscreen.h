#ifndef AUDIOOPTIONSSCREEN
#define AUDIOOPTIONSSCREEN

#include <optional>

#include "components/menu.h"
#include "screens/menuscreen.h"
#include "screens/screen.h"

class AudioOptionsScreen : public MenuScreen {
public:
    explicit AudioOptionsScreen(Context context);

    void redraw() override;

private:
    struct {
        MenuItem* audio_enabled {};
        MenuItem* volume {};
    } items {};

    void on_change_audio_enabled();

    void on_decrease_volume();
    void on_increase_volume();
};

#endif // AUDIOOPTIONSSCREEN
