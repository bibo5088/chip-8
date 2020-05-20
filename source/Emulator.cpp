#include "Emulator.h"

#include <algorithm>

#include "Font.h"

Emulator::Emulator() { reset(); }

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

void Emulator::emulate_cycle() {
  // Fetch opcode
  uint16_t opcode = memory[pc] << 8 | memory[pc + 1];

  execute_opcode(opcode);

  // Tick timers
  if (delay_timer > 0) {
    delay_timer--;
  }
  if (sound_timer > 0) {
    sound_timer--;
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
  }
}

void Emulator::instruction_00E0() {
  graphic.fill(0);
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
