#include <map>

#include "raylib.h"
#include <wrapper.hpp>

using namespace std::chrono_literals;

#define MAX_SAMPLES_PER_UPDATE 2048

SoundOutputState *soundOutputState;
SoundEvent lastEvent;
unsigned int offset;
bool first = true;

void update_source() {
    if (!soundOutputState->queue_.empty()) {
        lastEvent = SoundEvent(soundOutputState->queue_.back());
        offset = 0;
        first = false;
    }
}

void AudioInputCallback(void *buffer, unsigned int frames) {
    auto *buf = (short *) buffer;
    for (int i = 0; i < frames * 2; i++) {
        buf[i] = 0x00;
    }

    if (first)
        update_source();
    else {
        auto *vals = (short *) lastEvent.buffer_.data();

        for (int i = 0; i < frames * 2 && offset < (lastEvent.size_ / 2); i++) {
            buf[i] = vals[offset++];
            if (offset >= lastEvent.size_ / 2) {
                update_source();
                vals = (short *) lastEvent.buffer_.data();
            }
        }
    }
}

int main() {
    const int screenWidth = 1280;
    const int screenHeight = 800;

    InitWindow(screenWidth, screenHeight, "Enhel - Headless KEGS Emulator");
    InitAudioDevice();
    SetAudioStreamBufferSizeDefault(MAX_SAMPLES_PER_UPDATE);

    SetTargetFPS(60);
    SetTraceLogLevel(LOG_NONE);
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    bool cursor_state = false;
    DisableCursor();
    SetExitKey(0);

    std::map<int, bool> keys_pressed;

    HeKegs::StartEmulator();

    // kegs fixed on 48kHz, 16 bit and stereo (2 channels)
    AudioStream audioStream = LoadAudioStream(48000, 16, 2);

    PlayAudioStream(audioStream);
    soundOutputState = HeKegs::GetSoundState();

    SetAudioStreamCallback(audioStream, AudioInputCallback);

    while (!WindowShouldClose()) {
        auto key_value = GetKeyPressed();
        while (key_value > 0) {
            keys_pressed[key_value] = true;
            HeKegs::PushKeyboardEvent(key_value, true);

            key_value = GetKeyPressed();
        }

        for (auto &[key, state]: keys_pressed) {
            if (state) {
                if (IsKeyUp(key)) {
                    HeKegs::PushKeyboardEvent(key, false);
                    keys_pressed[key] = false;
                }
            }
        }

        if (IsKeyPressed(KEY_ESCAPE) && !cursor_state) {
            EnableCursor();
            cursor_state = true;
        }

        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && cursor_state) {
            DisableCursor();

            cursor_state = false;
        }

        if (!cursor_state) {
            auto mousePosition = GetMouseDelta();

            auto deltaX = (int) (mousePosition.x * 0.33f);
            auto deltaY = (int) (mousePosition.y * 0.33f);

            HeKegs::SetMouseDelta(deltaX, deltaY);
            HeKegs::SetMouseClick(IsMouseButtonDown(MouseButton::MOUSE_BUTTON_LEFT));
        }

        auto data = HeKegs::GetImageData();

        if (!data.ready_) continue;

        int format;
        switch (data.depth_) {
            case 32:
                format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
                break;
            case 24:
            default:
                format = PIXELFORMAT_UNCOMPRESSED_R8G8B8;
        }

        int width = data.width_;
        int height = data.height_;
        int size = data.data_.size();

        for (std::size_t i = 0; i < size; i += 4) {
            auto r = data.data_[i + 2];
            auto g = data.data_[i + 1];
            auto b = data.data_[i + 0];

            // swap r and b channels and set alpha channel to 0xFF
            data.data_[i + 0] = r;
            data.data_[i + 1] = g;
            data.data_[i + 2] = b;
            data.data_[i + 3] = 255;
        }

        Image checkedIm = {
                .data = data.data_.data(),
                .width = width,
                .height = height,
                .mipmaps = 1,
                .format = format
        };

        Texture2D checked = LoadTextureFromImage(checkedIm);

        BeginDrawing();

        ClearBackground(BLACK);

        DrawTexturePro(checked,
                       Rectangle{0, 0, static_cast<float>(checkedIm.width), static_cast<float>(checkedIm.height)},
                       Rectangle{0, 0, static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())},
                       Vector2{0, 0}, 0.0f, WHITE);

        EndDrawing();

        UnloadTexture(checked);
    }

    CloseWindow();

    return 0;
}
