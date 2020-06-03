#include "Emulator.h"

#include <algorithm>
#include <cassert>

#include "Font.h"

Emulator::Emulator() : rng_engine(std::random_device()()), rng_distribution(0, 255) { reset(); }

void Emulator::reset() {
  pc = 0x200;  // Program counter starts at 0x200

  // Reset registers to 0
  V.fill(0);
  I = 0;

  // Empty the stack
  while (!stack.empty()) {
    stack.pop();
  }

  // Clear memory
  memory.fill(0);

  // Clear graphics
  graphic.fill(0);

  // Load font into memory (starting from address 0)
  std::copy(chip8_font.begin(), chip8_font.end(), memory.begin());

  // Reset timers
  sound_timer = 0;
  delay_timer = 0;
}

void Emulator::load_rom(std::istream& rom) {
  for (auto i = 0x200; !rom.eof(); i++) {
    rom >> memory[i];
  }
}

void Emulator::emulate_cycle() {
  if (!waiting_for_key) {
    // Fetch opcode
    uint16_t opcode = memory[pc] << 8 | memory[pc + 1];

    execute_opcode(opcode);
  }
  // Tick timers
  if (delay_timer > 0) {
    delay_timer--;
  }
  if (sound_timer > 0) {
    sound_timer--;

    if (sound_timer == 0) {
      sound_flag = true;
    }
  }
}

void Emulator::execute_opcode(uint16_t opcode) {
  switch (opcode & 0xF000) {
    case 0x0000: {
      switch (opcode & 0x000F) {
        case 0x0000:
          instruction_00E0();
          break;
        case 0x000E:
          instruction_00EE();
          break;
      }
      break;
    }

    case 0x1000:
      instruction_1NNN(opcode & 0x0FFF);
      break;

    case 0x2000:
      instruction_2NNN(opcode & 0x0FFF);
      break;

    case 0x3000:
      instruction_3XNN((opcode & 0x0F00) >> 8, opcode & 0x00FF);
      break;

    case 0x4000:
      instruction_4XNN((opcode & 0x0F00) >> 8, opcode & 0x00FF);
      break;

    case 0x5000:
      instruction_5XY0((opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4);
      break;

    case 0x6000:
      instruction_6XNN((opcode & 0x0F00) >> 8, opcode & 0x00FF);
      break;

    case 0x7000:
      instruction_7XNN((opcode & 0x0F00) >> 8, opcode & 0x00FF);
      break;

    case 0x8000: {
      const uint8_t x = (opcode & 0x0F00) >> 8;
      const uint8_t y = (opcode & 0x00F0) >> 4;

      switch (opcode & 0x000F) {
        case 0x0000:
          instruction_8XY0(x, y);
          break;
        case 0x0001:
          instruction_8XY1(x, y);
          break;
        case 0x0002:
          instruction_8XY2(x, y);
          break;
        case 0x0003:
          instruction_8XY3(x, y);
          break;
        case 0x0004:
          instruction_8XY4(x, y);
          break;
        case 0x0005:
          instruction_8XY5(x, y);
          break;
        case 0x0006:
          instruction_8XY6(x);
          break;
        case 0x0007:
          instruction_8XY7(x, y);
          break;
        case 0x000E:
          instruction_8XYE(x);
          break;
      }
      break;
    }

    case 0x9000:
      instruction_9XY0((opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4);
      break;

    case 0xA000:
      instruction_ANNN((opcode & 0x0FFF));
      break;

    case 0xB000:
      instruction_BNNN((opcode & 0x0FFF));
      break;

    case 0xC000:
      instruction_CXNN((opcode & 0x0F00) >> 8, opcode & 0x00FF);
      break;

    case 0xD000:
      instruction_DXYN((opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4, opcode & 0x000F);
      break;

    case 0xE000: {
      switch (opcode & 0x000F) {
        case 0x000E:
          instruction_EX9E((opcode & 0x0F00) >> 8);
          break;
        case 0x0001:
          instruction_EXA1((opcode & 0x0F00) >> 8);
          break;
      }
      break;
    }

    case 0xF000: {
      const uint8_t reg = (opcode & 0x0F00) >> 8;

      switch (opcode & 0x00FF) {
        case 0x0007:
          instruction_FX07(reg);
          break;

        case 0x000A:
          instruction_FX0A(reg);
          break;

        case 0x0015:
          instruction_FX15(reg);
          break;

        case 0x0018:
          instruction_FX18(reg);
          break;

        case 0x001E:
          instruction_FX1E(reg);
          break;

        case 0x0029:
          instruction_FX29(reg);
          break;

        case 0x0033:
          instruction_FX33(reg);
          break;

        case 0x0055:
          instruction_FX55(reg);
          break;

        case 0x0065:
          instruction_FX65(reg);
          break;
      }
      break;
    }
  }
}

void Emulator::press_key(uint8_t key) {
  assert(key <= 0xF);

  keys[key] = true;
  if (waiting_for_key) {
    V[waiting_for_key_register] = key;
    waiting_for_key = false;
  }
}
void Emulator::release_key(uint8_t key) {
  assert(key <= 0xF);

  keys[key] = false;
}

bool Emulator::should_draw() {
  auto result = draw_flag;

  draw_flag = false;

  return result;
}
bool Emulator::should_buzz() {
  auto result = sound_flag;

  sound_flag = false;

  return result;
}

const std::array<uint8_t, 64 * 32>& Emulator::get_graphic() const { return graphic; }

void Emulator::instruction_00E0() {
  graphic.fill(0);
  draw_flag = true;
  pc += 2;
}
void Emulator::instruction_00EE() {
  pc = stack.top();
  stack.pop();
}
void Emulator::instruction_1NNN(uint16_t jump_address) { pc = jump_address; }
void Emulator::instruction_2NNN(uint16_t subroutine_address) {
  stack.push(pc + 2);
  pc = subroutine_address;
}
void Emulator::instruction_3XNN(uint8_t reg, uint8_t number) { pc += V[reg] == number ? 4 : 2; }
void Emulator::instruction_4XNN(uint8_t reg, uint8_t number) { pc += V[reg] != number ? 4 : 2; }
void Emulator::instruction_5XY0(uint8_t reg1, uint8_t reg2) { pc += V[reg1] == V[reg2] ? 4 : 2; }
void Emulator::instruction_6XNN(uint8_t reg, uint8_t value) {
  V[reg] = value;
  pc += 2;
}
void Emulator::instruction_7XNN(uint8_t reg, uint8_t value) {
  V[reg] += value;
  pc += 2;
}
void Emulator::instruction_8XY0(uint8_t reg1, uint8_t reg2) {
  V[reg1] = V[reg2];
  pc += 2;
}
void Emulator::instruction_8XY1(uint8_t reg1, uint8_t reg2) {
  V[reg1] |= V[reg2];
  pc += 2;
}
void Emulator::instruction_8XY2(uint8_t reg1, uint8_t reg2) {
  V[reg1] &= V[reg2];
  pc += 2;
}
void Emulator::instruction_8XY3(uint8_t reg1, uint8_t reg2) {
  V[reg1] ^= V[reg2];
  pc += 2;
}
void Emulator::instruction_8XY4(uint8_t reg1, uint8_t reg2) {
  // Carry if overflow
  V[0xF] = V[reg2] > std::numeric_limits<uint8_t>::max() - V[reg1] ? 1 : 0;
  V[reg1] += V[reg2];
  pc += 2;
}
void Emulator::instruction_8XY5(uint8_t reg1, uint8_t reg2) {
  // Borrow
  V[0xF] = V[reg1] > V[reg2] ? 1 : 0;
  V[reg1] -= V[reg2];
  pc += 2;
}
void Emulator::instruction_8XY6(uint8_t reg) {
  V[0xF] = V[reg] & 0x1;
  V[reg] >>= 1;
  pc += 2;
}
void Emulator::instruction_8XY7(uint8_t reg1, uint8_t reg2) {
  // Borrow
  V[0xF] = V[reg2] > V[reg1] ? 1 : 0;
  V[reg1] = V[reg2] - V[reg1];
  pc += 2;
}
void Emulator::instruction_8XYE(uint8_t reg) {
  V[0xF] = V[reg] & 0x1;
  V[reg] <<= 1;
  pc += 2;
}
void Emulator::instruction_9XY0(uint8_t reg1, uint8_t reg2) { pc += V[reg1] != V[reg2] ? 4 : 2; }
void Emulator::instruction_ANNN(uint16_t value) {
  I = value;
  pc += 2;
}
void Emulator::instruction_BNNN(uint16_t jump_address) { pc = V[0] + jump_address; }
void Emulator::instruction_CXNN(uint8_t reg, uint8_t value) {
  V[reg] = rng_distribution(rng_engine) & value;
  pc += 2;
}
void Emulator::instruction_DXYN(uint8_t reg1, uint8_t reg2, uint8_t height) {
  const auto x = V[reg1];
  const auto y = V[reg2];
  V[0xF] = 0;

  for (int yline = 0; yline < height; yline++) {
    const auto pixel = memory[I + yline];
    for (int xline = 0; xline < 8; xline++) {
      // Check if xlineTH bit of pixel is set to 1
      if ((pixel & (0b10000000 >> xline)) != 0) {
        const auto position = x + xline + ((y + yline) * 64);
        // Set the flag to 1 in case of collision
        if (graphic[position] == 1) {
          V[0xF] = 1;
        }
        graphic[position] ^= 1;
      }
    }
  }
  draw_flag = true;
  pc += 2;
}
void Emulator::instruction_EX9E(uint8_t key) { pc += keys[key] ? 4 : 2; }
void Emulator::instruction_EXA1(uint8_t key) { pc += keys[key] ? 2 : 4; }
void Emulator::instruction_FX07(uint8_t reg) {
  V[reg] = delay_timer;
  pc += 2;
}
void Emulator::instruction_FX0A(uint8_t reg) {
  waiting_for_key = true;
  waiting_for_key_register = reg;
  pc += 2;
}
void Emulator::instruction_FX15(uint8_t reg) {
  delay_timer = V[reg];
  pc += 2;
}
void Emulator::instruction_FX18(uint8_t reg) {
  sound_timer = V[reg];
  pc += 2;
}
void Emulator::instruction_FX1E(uint8_t reg) {
  I += V[reg];
  V[0xF] = I > 0xFFF ? 1 : 0;
  pc += 2;
}
void Emulator::instruction_FX29(uint8_t reg) {
  I = V[reg] * 5;  // Font is loader in memory at address 0 and each character is 5 ints
  pc += 2;
}
void Emulator::instruction_FX33(uint8_t reg) {
  memory[I] = V[reg] / 100;
  memory[I + 1] = (V[reg] / 10) % 10;
  memory[I + 2] = (V[reg] % 100) % 10;
  pc += 2;
}
void Emulator::instruction_FX55(uint8_t reg) {
  for (auto i = 0; i <= reg; i++) {
    memory[I + i] = V[i];
  }
  pc += 2;
}
void Emulator::instruction_FX65(uint8_t reg) {
  for (auto i = 0; i <= reg; i++) {
    V[i] = memory[I + i];
  }
  pc += 2;
}

