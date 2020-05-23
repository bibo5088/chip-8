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

  SUBCASE("00E0 should clear graphics and increment the program counter counter by 2") {
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

  SUBCASE(
      "6XNN should set the value of V[X] to NN and increment the program counter counter by 2") {
    emulator.pc = 10;
    emulator.V[5] = 10;

    emulator.execute_opcode(0x6504);

    CHECK(emulator.V[5] == 4);

    CHECK(emulator.pc == 12);
  }

  SUBCASE("7XNN should add NN to V[X] and increment the program counter counter by 2") {
    emulator.pc = 10;
    emulator.V[5] = 10;

    emulator.execute_opcode(0x7504);

    CHECK(emulator.V[5] == 14);

    CHECK(emulator.pc == 12);
  }

  SUBCASE(
      "7XNN should add NN to V[X] without modifying the carry flag and increment the program "
      "counter counter by 2") {
    emulator.pc = 10;
    emulator.V[5] = 250;
    emulator.V[0xF] = 0;

    emulator.execute_opcode(0x7506);

    CHECK(emulator.V[5] == 0);
    CHECK(emulator.V[0xF] == 0);

    CHECK(emulator.pc == 12);
  }

  SUBCASE(
      "8XY0 should set the value of V[X] to V[Y] and increment the program counter counter by 2") {
    emulator.pc = 10;
    emulator.V[1] = 10;
    emulator.V[2] = 30;

    emulator.execute_opcode(0x8120);

    CHECK(emulator.V[1] == 30);

    CHECK(emulator.pc == 12);
  }

  SUBCASE("8XY1 should bitwise OR V[Y] to V[X] and increment the program counter counter by 2") {
    emulator.pc = 10;
    emulator.V[1] = 0b1010;
    emulator.V[2] = 0b0011;

    emulator.execute_opcode(0x8121);

    CHECK(emulator.V[1] == 0b1011);

    CHECK(emulator.pc == 12);
  }

  SUBCASE("8XY2 should bitwise AND V[Y] to V[X] and increment the program counter counter by 2") {
    emulator.pc = 10;
    emulator.V[1] = 0b1010;
    emulator.V[2] = 0b0011;

    emulator.execute_opcode(0x8122);

    CHECK(emulator.V[1] == 0b0010);

    CHECK(emulator.pc == 12);
  }

  SUBCASE("8XY3 should bitwise XOR V[Y] to V[X] and increment the program counter counter by 2") {
    emulator.pc = 10;
    emulator.V[1] = 0b1010;
    emulator.V[2] = 0b0011;

    emulator.execute_opcode(0x8123);

    CHECK(emulator.V[1] == 0b1001);

    CHECK(emulator.pc == 12);
  }

  SUBCASE("8XY4 should add V[Y] to V[X] and increment the program counter counter by 2") {
    emulator.pc = 10;
    emulator.V[1] = 10;
    emulator.V[2] = 20;

    emulator.execute_opcode(0x8124);

    CHECK(emulator.V[1] == 30);
    CHECK(emulator.V[0xF] == 0);

    CHECK(emulator.pc == 12);
  }

  SUBCASE(
      "8XY4 should add V[Y] to V[X], set V[0xF] to 1 if there's a carry and increment the program "
      "counter counter by 2") {
    emulator.pc = 10;
    emulator.V[1] = 250;
    emulator.V[2] = 6;

    emulator.execute_opcode(0x8124);

    CHECK(emulator.V[1] == 0);
    CHECK(emulator.V[0xF] == 1);

    CHECK(emulator.pc == 12);
  }

  SUBCASE(
      "8XY5 should subtract V[Y] to V[X], set V[0xF] to 1 if there's no borrow and increment the "
      "program counter counter by 2") {
    emulator.pc = 10;
    emulator.V[1] = 20;
    emulator.V[2] = 10;

    emulator.execute_opcode(0x8125);

    CHECK(emulator.V[1] == 10);
    CHECK(emulator.V[0xF] == 1);

    CHECK(emulator.pc == 12);
  }

  SUBCASE(
      "8XY5 should subtract V[Y] to V[X], set V[0xF] to 0 if there's a borrow and increment the "
      "program counter counter by 2") {
    emulator.pc = 10;
    emulator.V[1] = 10;
    emulator.V[2] = 20;

    emulator.execute_opcode(0x8125);

    CHECK(emulator.V[1] == 246);
    CHECK(emulator.V[0xF] == 0);

    CHECK(emulator.pc == 12);
  }

  SUBCASE(
      "8XY6 should set V[0xF] to V[X]'s least significant bit, shift V[X] to the right by one and "
      "increment the program counter counter by 2") {
    emulator.pc = 10;
    emulator.V[0xA] = 0b101;
    emulator.V[0xF] = 0;

    emulator.execute_opcode(0x8A06);

    CHECK(emulator.V[0xA] == 0b10);
    CHECK(emulator.V[0xF] == 1);

    CHECK(emulator.pc == 12);
  }

  SUBCASE(
      "8XY7 should subtract V[X] to V[Y] and store the result in V[X], set V[0xF] to 1 if there's "
      "no borrow and increment the program counter counter by 2") {
    emulator.pc = 10;
    emulator.V[1] = 10;
    emulator.V[2] = 20;

    emulator.execute_opcode(0x8127);

    CHECK(emulator.V[1] == 10);
    CHECK(emulator.V[0xF] == 1);

    CHECK(emulator.pc == 12);
  }

  SUBCASE(
      "8XY7 should subtract V[X] to V[Y] and store the result in V[X], set V[0xF] to 0 if there's "
      "a borrow and increment the program counter counter by 2") {
    emulator.pc = 10;
    emulator.V[1] = 20;
    emulator.V[2] = 10;

    emulator.execute_opcode(0x8127);

    CHECK(emulator.V[1] == 246);
    CHECK(emulator.V[0xF] == 0);

    CHECK(emulator.pc == 12);
  }

  SUBCASE(
      "8XE6 should set V[0xF] to V[X]'s least significant bit, shift V[X] to the left by one and "
      "increment the program counter counter by 2") {
    emulator.pc = 10;
    emulator.V[0xA] = 0b101;
    emulator.V[0xF] = 0;

    emulator.execute_opcode(0x8A0E);

    CHECK(emulator.V[0xA] == 0b1010);
    CHECK(emulator.V[0xF] == 1);

    CHECK(emulator.pc == 12);
  }

  SUBCASE("9XY0 should increment the program counter by 4 if V[X] != V[Y]") {
    emulator.pc = 10;
    emulator.V[1] = 0xAA;
    emulator.V[2] = 0xAB;

    emulator.execute_opcode(0x9120);

    CHECK(emulator.pc == 14);
  }

  SUBCASE("9XY0 should increment the program counter by 2 if V[X] == V[Y]") {
    emulator.pc = 10;
    emulator.V[1] = 0xAA;
    emulator.V[2] = 0xAA;

    emulator.execute_opcode(0x9120);

    CHECK(emulator.pc == 12);
  }

  SUBCASE("ANNN should set I to NNN and increment the program counter counter by 2") {
    emulator.pc = 10;
    emulator.I = 0xABC;

    emulator.execute_opcode(0xADEF);

    CHECK(emulator.pc == 12);
    CHECK(emulator.I == 0xDEF);
  }

  SUBCASE("BNNN should set the program counter counter to V[0] + NNN") {
    emulator.pc = 10;
    emulator.V[0] = 5;

    emulator.execute_opcode(0xB004);

    CHECK(emulator.pc == 9);
  }

  SUBCASE(
      "CNNN should set V[X] to the bitwise AND of a random integer between 0 and 255 and NN and "
      "increment the program counter counter by 2") {
    emulator.pc = 10;
    emulator.V[1] = 1;
    emulator.rng_engine.seed(145);  // Mocked for the test

    emulator.execute_opcode(0xC10F);

    CHECK(emulator.pc == 12);
    CHECK(emulator.V[1] == 5);
  }

  SUBCASE(
      "DXYN should draw a sprite at location V[X], V[Y] with a height of N, set the flag to 0 if "
      "there's not collision and increment the program counter counter by 2") {
    emulator.pc = 10;
    emulator.V[1] = 0;
    emulator.V[2] = 0;

    emulator.I = 0x300;
    emulator.memory[0x300] = 0b01111110;
    emulator.memory[0x300 + 1] = 0b10000001;
    emulator.memory[0x300 + 2] = 0b01111110;

    emulator.execute_opcode(0xD123);

    CHECK(emulator.pc == 12);
    CHECK(emulator.V[0xF] == 0);

    constexpr auto get_position = [](int x, int y) -> int { return x + y * 64; };

    // 0b01111110
    CHECK(emulator.graphic[get_position(0, 0)] == 0);
    CHECK(emulator.graphic[get_position(1, 0)] == 1);
    CHECK(emulator.graphic[get_position(2, 0)] == 1);
    CHECK(emulator.graphic[get_position(3, 0)] == 1);
    CHECK(emulator.graphic[get_position(4, 0)] == 1);
    CHECK(emulator.graphic[get_position(5, 0)] == 1);
    CHECK(emulator.graphic[get_position(6, 0)] == 1);
    CHECK(emulator.graphic[get_position(7, 0)] == 0);
    // 0b10000001
    CHECK(emulator.graphic[get_position(0, 1)] == 1);
    CHECK(emulator.graphic[get_position(1, 1)] == 0);
    CHECK(emulator.graphic[get_position(2, 1)] == 0);
    CHECK(emulator.graphic[get_position(3, 1)] == 0);
    CHECK(emulator.graphic[get_position(4, 1)] == 0);
    CHECK(emulator.graphic[get_position(5, 1)] == 0);
    CHECK(emulator.graphic[get_position(6, 1)] == 0);
    CHECK(emulator.graphic[get_position(7, 1)] == 1);
    // 0b01111110
    CHECK(emulator.graphic[get_position(0, 2)] == 0);
    CHECK(emulator.graphic[get_position(1, 2)] == 1);
    CHECK(emulator.graphic[get_position(2, 2)] == 1);
    CHECK(emulator.graphic[get_position(3, 2)] == 1);
    CHECK(emulator.graphic[get_position(4, 2)] == 1);
    CHECK(emulator.graphic[get_position(5, 2)] == 1);
    CHECK(emulator.graphic[get_position(6, 2)] == 1);
    CHECK(emulator.graphic[get_position(7, 2)] == 0);
  }

  SUBCASE(
      "DXYN should draw a sprite at location V[X], V[Y] with a height of N, set the flag to 1 if "
      "there's not collision and increment the program counter counter by 2") {
    emulator.pc = 10;
    emulator.V[1] = 0;
    emulator.V[2] = 0;

    emulator.I = 0x300;
    emulator.memory[0x300] = 0b01111110;
    emulator.memory[0x300 + 1] = 0b10000001;
    emulator.memory[0x300 + 2] = 0b01111110;

    // Mock collision
    emulator.graphic[1] = 1;

    emulator.execute_opcode(0xD123);

    CHECK(emulator.pc == 12);
    CHECK(emulator.V[0xF] == 1);

    constexpr auto get_position = [](int x, int y) -> int { return x + y * 64; };

    // 0b01111110
    CHECK(emulator.graphic[get_position(0, 0)] == 0);
    CHECK(emulator.graphic[get_position(1, 0)] == 0);  // Unset due to collision
    CHECK(emulator.graphic[get_position(2, 0)] == 1);
    CHECK(emulator.graphic[get_position(3, 0)] == 1);
    CHECK(emulator.graphic[get_position(4, 0)] == 1);
    CHECK(emulator.graphic[get_position(5, 0)] == 1);
    CHECK(emulator.graphic[get_position(6, 0)] == 1);
    CHECK(emulator.graphic[get_position(7, 0)] == 0);
    // 0b10000001
    CHECK(emulator.graphic[get_position(0, 1)] == 1);
    CHECK(emulator.graphic[get_position(1, 1)] == 0);
    CHECK(emulator.graphic[get_position(2, 1)] == 0);
    CHECK(emulator.graphic[get_position(3, 1)] == 0);
    CHECK(emulator.graphic[get_position(4, 1)] == 0);
    CHECK(emulator.graphic[get_position(5, 1)] == 0);
    CHECK(emulator.graphic[get_position(6, 1)] == 0);
    CHECK(emulator.graphic[get_position(7, 1)] == 1);
    // 0b01111110
    CHECK(emulator.graphic[get_position(0, 2)] == 0);
    CHECK(emulator.graphic[get_position(1, 2)] == 1);
    CHECK(emulator.graphic[get_position(2, 2)] == 1);
    CHECK(emulator.graphic[get_position(3, 2)] == 1);
    CHECK(emulator.graphic[get_position(4, 2)] == 1);
    CHECK(emulator.graphic[get_position(5, 2)] == 1);
    CHECK(emulator.graphic[get_position(6, 2)] == 1);
    CHECK(emulator.graphic[get_position(7, 2)] == 0);
  }
}