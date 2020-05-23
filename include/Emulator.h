#ifndef CHIP8EMUTESTS_EMULATOR_H
#define CHIP8EMUTESTS_EMULATOR_H

#include <array>
#include <cinttypes>
#include <limits>
#include <random>
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

  std::array<uint8_t, 16> V;   // Registers
  uint16_t I;                  // Index register
  uint16_t pc;                 // Program counter
  std::stack<uint16_t> stack;  // Used for function calls

  // When set above zero the timers will count down to zero.
  uint8_t delay_timer;
  uint8_t sound_timer;  // Will make the system buzz sound when it reaches zero.

  // RNG for instruction CXNN
  std::mt19937 rng_engine;
  std::uniform_int_distribution<> rng_distribution;

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
  // 6XNN Sets VX to NN.
  void instruction_6XNN(uint8_t reg, uint8_t value);
  // 7XNN Adds NN to VX. (Carry flag is not changed)
  void instruction_7XNN(uint8_t reg, uint8_t value);
  // 8XY0 Sets VX to the value of VY.
  void instruction_8XY0(uint8_t reg1, uint8_t reg2);
  // 8XY1 Sets VX to VX or VY. (Bitwise OR operation)
  void instruction_8XY1(uint8_t reg1, uint8_t reg2);
  // 8XY2 Sets VX to VX and VY. (Bitwise AND operation)
  void instruction_8XY2(uint8_t reg1, uint8_t reg2);
  // 8XY3 Sets VX to VX xor VY.
  void instruction_8XY3(uint8_t reg1, uint8_t reg2);
  // 8XY4 Adds VY to VX. VF is set to 1 when there's a carry, and to 0 when there isn't.
  void instruction_8XY4(uint8_t reg1, uint8_t reg2);
  // 8XY5 VY is subtracted from VX. VF is set to 0 when there's a borrow, and 1 when there isn't.
  void instruction_8XY5(uint8_t reg1, uint8_t reg2);
  // 8XY6 Stores the least significant bit of VX in VF and then shifts VX to the right by 1.
  void instruction_8XY6(uint8_t reg);
  // 8XY7 Sets VX to VY minus VX. VF is set to 0 when there's a borrow, and 1 when there isn't.
  void instruction_8XY7(uint8_t reg1, uint8_t reg2);
  // 8XYE Stores the most significant bit of VX in VF and then shifts VX to the left by 1
  void instruction_8XYE(uint8_t reg);
  // 9XY0 Skips the next instruction if VX doesn't equal VY.
  void instruction_9XY0(uint8_t reg1, uint8_t reg2);
  // ANNN Sets I to the address NNN.
  void instruction_ANNN(uint16_t value);
  // BNNN Jumps to the address NNN plus V0.
  void instruction_BNNN(uint16_t jump_address);
  // CXNN Sets VX to the result of a bitwise and operation on a random number (Typically: 0 to 255)
  // and NN.
  void instruction_CXNN(uint8_t reg, uint8_t value);
  // Draws a sprite at coordinate (VX, VY) that has a width of 8 pixels and a height of N pixels.
  // Each row of 8 pixels is read as bit-coded starting from memory location I; I value doesn’t
  // change after the execution of this instruction. As described above, VF is set to 1 if any
  // screen pixels are flipped from set to unset when the sprite is drawn, and to 0 if that doesn’t
  // happen.
  void instruction_DXYN(uint8_t reg1, uint8_t reg2, uint8_t height);
};

#endif  // CHIP8EMUTESTS_EMULATOR_H
