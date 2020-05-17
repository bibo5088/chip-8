#ifndef CHIP8EMUTESTS_EMULATOR_H
#define CHIP8EMUTESTS_EMULATOR_H

#include <array>
#include <cinttypes>
#include <stack>

class Emulator {
public:
  Emulator();

public:
  void reset();

  void emulate_cycle();

  void execute_opcode(uint16_t opcode);

  std::array<uint8_t, 4096> memory;
  std::array<uint8_t, 64 * 32> graphic;  // 2048 pixel screen

  std::array<uint8_t, 16> V;  // Registers
  uint16_t I;                 // Index register
  uint16_t pc;                // Program counter
  std::stack<uint16_t> stack;  // Used for function calls

  // When set above zero the timers will count down to zero.
  uint8_t delay_timer;
  uint8_t sound_timer;  // Will make the system buzz sound when it reaches zero.

private:
  // Instructions //

  // 00E0 Clears the screen.
  void instruction_00E0();
  // 00EE Returns from a subroutine.
  void instruction_00EE();
  // 1NNN Returns from a subroutine.
  void instruction_1NNN(uint16_t jump_address);
  // 2NNN Calls subroutine at NNN.
  void instruction_2NNN(uint16_t subroutine_address);
  // 3XNN Skips the next instruction if VX equals NN.
  void instruction_3XNN(uint8_t reg, uint8_t number);
  // 4XNN Skips the next instruction if VX doesn't NN.
  void instruction_4XNN(uint8_t reg, uint8_t number);
  // 5XY0 Skips the next instruction if VX equals VY.
  void instruction_5XY0(uint8_t reg1, uint8_t reg2);
};

#endif  // CHIP8EMUTESTS_EMULATOR_H
