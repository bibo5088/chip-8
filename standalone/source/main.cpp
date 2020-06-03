#include <SDL.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>

#include "Emulator.h"

constexpr uint8_t NO_KEY_MATCHED = 255;

/**
Keypad                   Keyboard
+-+-+-+-+                +-+-+-+-+
|1|2|3|C|                |1|2|3|4|
+-+-+-+-+                +-+-+-+-+
|4|5|6|D|                |Q|W|E|R|
+-+-+-+-+       =>       +-+-+-+-+
|7|8|9|E|                |A|S|D|F|
+-+-+-+-+                +-+-+-+-+
|A|0|B|F|                |Z|X|C|V|
+-+-+-+-+                +-+-+-+-+
 Returns NO_KEY_MATCHED if no key matched
 */
uint8_t scancode_to_chip8_key(SDL_Scancode scancode) {
  switch (scancode) {
      // First row
    case SDL_SCANCODE_1:
      return 1;
    case SDL_SCANCODE_2:
      return 2;
    case SDL_SCANCODE_3:
      return 3;
    case SDL_SCANCODE_4:
      return 0xC;

      // Second row
    case SDL_SCANCODE_Q:
      return 4;
    case SDL_SCANCODE_W:
      return 5;
    case SDL_SCANCODE_E:
      return 6;
    case SDL_SCANCODE_R:
      return 0xD;

      // Third row
    case SDL_SCANCODE_A:
      return 7;
    case SDL_SCANCODE_S:
      return 8;
    case SDL_SCANCODE_D:
      return 9;
    case SDL_SCANCODE_F:
      return 0xE;

      // Fourth row
    case SDL_SCANCODE_Z:
      return 0xA;
    case SDL_SCANCODE_X:
      return 0;
    case SDL_SCANCODE_C:
      return 0xB;
    case SDL_SCANCODE_V:
      return 0xF;

    default:
      return NO_KEY_MATCHED;
  }
}

constexpr int PIXEL_SIZE = 16;

void draw(SDL_Renderer *renderer, const std::array<uint8_t, 64 * 32> &graphics) {
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);  // Black
  SDL_RenderClear(renderer);

  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);  // White

  for (auto y = 0; y < 32; y++) {
    for (auto x = 0; x < 64; x++) {
      // Only draw white if pixel is equal to 1
      if (graphics[x + y * 64] == 1) {
        SDL_Rect pixel;
        pixel.x = x * PIXEL_SIZE;
        pixel.y = y * PIXEL_SIZE;
        pixel.w = PIXEL_SIZE;
        pixel.h = PIXEL_SIZE;

        SDL_RenderFillRect(renderer, &pixel);
      }
    }
  }

  SDL_RenderPresent(renderer);
}

constexpr auto milliseconds_per_frame = std::chrono::milliseconds(1000 / 60);

int main(int argc, char **argv) {
  // Check if rom exist
  if (argc < 2) {
    std::cerr << "Missing rom";
    return 1;
  }
  if (!std::filesystem::exists(argv[1])) {
    std::cerr << "File: " << argv[1] << " does not exist";
    return 1;
  }

  // Window setup
  SDL_Init(SDL_INIT_VIDEO);

  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_CreateWindowAndRenderer(64 * PIXEL_SIZE, 32 * PIXEL_SIZE, 0, &window, &renderer);

  // Emulator and rom setup
  Emulator emulator;

  std::ifstream rom(argv[1], std::ios::binary);
  emulator.load_rom(rom);

  SDL_Event event;
  // Emulation loop
  while (true) {
    auto start_of_frame = std::chrono::system_clock::now();

    // Handle quit and key press/release events
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_QUIT:
          goto quit;

        case SDL_KEYDOWN: {
          auto key = scancode_to_chip8_key(event.key.keysym.scancode);
          if (key != NO_KEY_MATCHED) {
            emulator.press_key(key);
          }

          break;
        }

        case SDL_KEYUP: {
          auto key = scancode_to_chip8_key(event.key.keysym.scancode);
          if (key != NO_KEY_MATCHED) {
            emulator.release_key(key);
          }

          break;
        }
      }
    }

    try {
      emulator.emulate_cycle();
    } catch (std::exception e) {
      std::cerr << e.what();
      goto quit;
    }

    if (emulator.should_draw()) {
      draw(renderer, emulator.get_graphic());
    }

    // Frame cap
    auto time_elapsed = std::chrono::system_clock::now() - start_of_frame;
    if (time_elapsed < milliseconds_per_frame) {
      std::this_thread::sleep_for(milliseconds_per_frame - time_elapsed);
    }
  }

quit:
  // Window cleanup
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);

  SDL_Quit();

  return 0;
}
