#include "Emulator.h"

#include <doctest/doctest.h>

#include "Font.h"

TEST_CASE("Emulator can be reset") {
  Emulator emulator;

  emulator.pc = 185;
  emulator.I = 1240;
  emulator.graphic[10] = 1;
  emulator.memory[100] = 1;
  emulator.stack.push(1);
  emulator.stack.push(2);
  emulator.stack.push(3);

  emulator.reset();

  SUBCASE("The program counter is set to 0x200") { CHECK(emulator.pc == 0x200); }
  SUBCASE("The registers are set to 0") {
    CHECK(emulator.V[0] == 0);
    CHECK(emulator.V[1] == 0);
    CHECK(emulator.V[2] == 0);
    CHECK(emulator.V[3] == 0);
    CHECK(emulator.V[4] == 0);
    CHECK(emulator.V[5] == 0);
    CHECK(emulator.V[6] == 0);
    CHECK(emulator.V[7] == 0);
    CHECK(emulator.V[8] == 0);
    CHECK(emulator.V[9] == 0);
    CHECK(emulator.V[10] == 0);
    CHECK(emulator.V[11] == 0);
    CHECK(emulator.V[12] == 0);
    CHECK(emulator.V[13] == 0);
    CHECK(emulator.V[14] == 0);
    CHECK(emulator.V[15] == 0);
  }
  SUBCASE("The Index register is set to 0") { CHECK(emulator.I == 0); }
  SUBCASE("The stack is emptied") { CHECK(emulator.stack.empty()); }
  SUBCASE("The memory and graphics are reset") {
    CHECK(emulator.graphic[10] == 0);
    CHECK(emulator.memory[100] == 0);
  }
  SUBCASE("The font is in memory") {
    for (int i = 0; i < chip8_font.size(); i++) {
      CHECK(emulator.memory[i] == chip8_font[i]);
    }
  }
  SUBCASE("The timers are set to 0") {
    CHECK(emulator.sound_timer == 0);
    CHECK(emulator.delay_timer == 0);
  }
}

TEST_CASE("Emulator can execute opcodes") {
  Emulator emulator;

  SUBCASE("00E0 should clear graphics and increment the programter counter by 2") {
    emulator.pc = 1;
    emulator.graphic[10] = 1;

    emulator.execute_opcode(0x00E0);

    CHECK(emulator.graphic[10] == 0);
    CHECK(emulator.pc == 3);
  }

  SUBCASE("00EE should set the program counter at the top of the stack and pop it") {
    emulator.stack.push(50);
    emulator.pc = 10;

    emulator.execute_opcode(0x00EE);
    CHECK(emulator.pc == 50);
    CHECK(emulator.stack.size() == 0);
  }

  SUBCASE("1NNN should set the program counter to NNN") {
    emulator.pc = 10;

    emulator.execute_opcode(0x1ABC);
    CHECK(emulator.pc == 0xABC);
  }

  SUBCASE("2NNN should push to the stack the next opcode and set the program counter to NNN") {
    emulator.pc = 10;

    emulator.execute_opcode(0x2ABC);
    CHECK(emulator.stack.top() == 12);
    CHECK(emulator.pc == 0xABC);
  }

  SUBCASE("3XNN should increment the program counter by 4 if V[X] == NN") {
    emulator.pc = 10;
    emulator.V[1] = 0xAA;

    emulator.execute_opcode(0x31AA);

    CHECK(emulator.pc == 14);
  }

  SUBCASE("3XNN should increment the program counter by 2 if V[X] != NN") {
    emulator.pc = 10;
    emulator.V[1] = 0xAA;

    emulator.execute_opcode(0x31AB);

    CHECK(emulator.pc == 12);
  }

  SUBCASE("4XNN should increment the program counter by 4 if V[X] != NN") {
    emulator.pc = 10;
    emulator.V[1] = 0xAA;

    emulator.execute_opcode(0x41AB);

    CHECK(emulator.pc == 14);
  }

  SUBCASE("4XNN should increment the program counter by 2 if V[X] == NN") {
    emulator.pc = 10;
    emulator.V[1] = 0xAA;

    emulator.execute_opcode(0x41AA);

    CHECK(emulator.pc == 12);
  }

  SUBCASE("5XY0 should increment the program counter by 4 if V[X] == V[Y]") {
    emulator.pc = 10;
    emulator.V[1] = 0xAA;
    emulator.V[2] = 0xAA;

    emulator.execute_opcode(0x5120);

    CHECK(emulator.pc == 14);
  }

  SUBCASE("5XY0 should increment the program counter by 2 if V[X] != V[Y]") {
    emulator.pc = 10;
    emulator.V[1] = 0xAA;
    emulator.V[2] = 0xAB;

    emulator.execute_opcode(0x5120);

    CHECK(emulator.pc == 12);
  }
}