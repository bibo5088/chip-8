#include "Emulator.h"

#include <doctest/doctest.h>

#include <istream>
#include <streambuf>

#include "Font.h"

class EmulatorTest : public Emulator {
public:
  using Emulator::delay_timer;
  using Emulator::draw_flag;
  using Emulator::graphic;
  using Emulator::I;
  using Emulator::keys;
  using Emulator::memory;
  using Emulator::pc;
  using Emulator::rng_distribution;
  using Emulator::rng_engine;
  using Emulator::sound_flag;
  using Emulator::sound_timer;
  using Emulator::stack;
  using Emulator::V;
  using Emulator::waiting_for_key;
  using Emulator::waiting_for_key_register;
};

// For testing using istream
struct membuf : std::streambuf {
  membuf(char const* base, size_t size) {
    char* p(const_cast<char*>(base));
    this->setg(p, p, p + size);
  }
};
struct imemstream : virtual membuf, std::istream {
  imemstream(char const* base, size_t size)
      : membuf(base, size), std::istream(static_cast<std::streambuf*>(this)) {}
};

TEST_CASE("Emulator can be reset") {
  EmulatorTest emulator;

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

TEST_CASE("Emulator can load a rom") {
  EmulatorTest emulator;

  std::array<uint8_t, 4> data{0x00, 0xE0, 0x61, 0x04};
  imemstream rom(reinterpret_cast<const char*>(data.data()), data.size());

  emulator.load_rom(rom);

  CHECK(emulator.memory[0x200] == 0x00);
  CHECK(emulator.memory[0x200 + 1] == 0xE0);
  CHECK(emulator.memory[0x200 + 2] == 0x61);
  CHECK(emulator.memory[0x200 + 3] == 0x04);
}

TEST_CASE("Emulator can handle key presses") {
  EmulatorTest emulator;

  SUBCASE("Pressing a key") {
    emulator.keys[1] = false;
    emulator.waiting_for_key = false;
    emulator.waiting_for_key_register = 5;
    emulator.V[5] = 50;

    SUBCASE("should set it's key to true") {
      emulator.press_key(1);

      CHECK(emulator.keys[1]);
      CHECK(!emulator.waiting_for_key);
      CHECK(emulator.V[5] == 50);
    }

    SUBCASE(
        "should set waiting_for_key to false and V[waiting_for_key_register] to the "
        "key if waiting_for_key is true") {
      emulator.waiting_for_key = true;

      emulator.press_key(1);

      CHECK(emulator.keys[1]);
      CHECK(!emulator.waiting_for_key);
      CHECK(emulator.V[5] == 1);
    }
  }

  SUBCASE("Releasing a key should set it's key to false") {
    emulator.keys[1] = true;

    emulator.release_key(1);

    CHECK(!emulator.keys[1]);
  }
}

TEST_CASE("Emulator can execute opcodes") {
  EmulatorTest emulator;

  SUBCASE(
      "00E0 should clear graphics, set the draw flag to true and increment the program counter "
      "counter by 2") {
    emulator.pc = 1;
    emulator.graphic[10] = 1;

    emulator.execute_opcode(0x00E0);

    CHECK(emulator.graphic[10] == 0);
    CHECK(emulator.draw_flag);
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

    emulator.rng_engine.seed(145);
    auto random = emulator.rng_distribution(emulator.rng_engine);
    emulator.rng_engine.seed(145);

    emulator.execute_opcode(0xC10F);

    CHECK(emulator.pc == 12);
    CHECK(emulator.V[1] == (random & 0x0F));
  }

  SUBCASE(
      "DXYN should draw a sprite at location V[X], V[Y] with a height of N, set the flag to 0 if "
      "there's not collision, set the draw flag to true and increment the program counter counter "
      "by 2") {
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
    CHECK(emulator.draw_flag);

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
      "there's not collision, set the draw flag to true and increment the program counter counter "
      "by 2") {
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
    CHECK(emulator.draw_flag);

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

  SUBCASE("EX9E should increment the program counter by 4 if key X is pressed") {
    emulator.pc = 10;
    emulator.keys[1] = true;

    emulator.execute_opcode(0xE19E);

    CHECK(emulator.pc == 14);
  }

  SUBCASE("EX9E should increment the program counter by 2 if key X is not pressed") {
    emulator.pc = 10;
    emulator.keys[1] = false;

    emulator.execute_opcode(0xE19E);

    CHECK(emulator.pc == 12);
  }

  SUBCASE("EXA1 should increment the program counter by 2 if key X is pressed") {
    emulator.pc = 10;
    emulator.keys[1] = true;

    emulator.execute_opcode(0xE1A1);

    CHECK(emulator.pc == 12);
  }

  SUBCASE("EXA1 should increment the program counter by 4 if key X is not pressed") {
    emulator.pc = 10;
    emulator.keys[1] = false;

    emulator.execute_opcode(0xE1A1);

    CHECK(emulator.pc == 14);
  }

  SUBCASE(
      "FX07 should set V[X] to the delay timer and increment the program counter counter by 2") {
    emulator.pc = 10;
    emulator.delay_timer = 100;
    emulator.V[4] = 47;

    emulator.execute_opcode(0xF407);

    CHECK(emulator.pc == 12);
    CHECK(emulator.V[4] == 100);
  }

  SUBCASE(
      "FX0A should set waiting_for_key to true and waiting_for_key_register to X and increment the "
      "program counter counter by 2") {
    emulator.pc = 10;
    emulator.waiting_for_key = false;
    emulator.waiting_for_key_register = 0xF;

    emulator.execute_opcode(0xF40A);

    CHECK(emulator.pc == 12);
    CHECK(emulator.waiting_for_key);
    CHECK(emulator.waiting_for_key_register == 4);
  }

  SUBCASE("FX15 should set the delay timer to V[X] and increment program counter counter by 2") {
    emulator.pc = 10;
    emulator.delay_timer = 0;
    emulator.V[8] = 60;

    emulator.execute_opcode(0xF815);

    CHECK(emulator.pc == 12);
    CHECK(emulator.delay_timer == 60);
  }

  SUBCASE("FX18 should set the sound  timer to V[X] and increment program counter counter by 2") {
    emulator.pc = 10;
    emulator.sound_timer = 0;
    emulator.V[8] = 60;

    emulator.execute_opcode(0xF818);

    CHECK(emulator.pc == 12);
    CHECK(emulator.sound_timer == 60);
  }

  SUBCASE(
      "FX1E should add V[X] to I, set set V[0xF] to 0 if there's no range overflow and increment "
      "program counter counter by 2") {
    emulator.pc = 10;
    emulator.I = 70;
    emulator.V[7] = 30;
    emulator.V[0xF] = 1;

    emulator.execute_opcode(0xF71E);

    CHECK(emulator.pc == 12);
    CHECK(emulator.I == 100);
    CHECK(emulator.V[0xF] == 0);
  }

  SUBCASE(
      "FX1E should add V[X] to I, set set V[0xF] to 1 if there's a range overflow and increment "
      "program counter counter by 2") {
    emulator.pc = 10;
    emulator.I = 0xFFE;
    emulator.V[7] = 10;
    emulator.V[0xF] = 0;

    emulator.execute_opcode(0xF71E);

    CHECK(emulator.pc == 12);
    CHECK(emulator.I == 4104);
    CHECK(emulator.V[0xF] == 1);
  }

  SUBCASE(
      "FX29 should set I to the location of the character V[X] (chip 8 font) and increment program "
      "counter counter by 2") {
    emulator.pc = 10;
    emulator.I = 80;
    emulator.V[0] = 5;

    emulator.execute_opcode(0xF029);

    CHECK(emulator.pc == 12);
    CHECK(emulator.I == 25);
  }

  SUBCASE(
      "FX33 should store the binary-coded decimal representation of V[X] in memory starting at "
      "address I and increment program counter counter by 2") {
    emulator.pc = 10;
    emulator.I = 100;
    emulator.V[0xE] = 254;

    emulator.execute_opcode(0xFE33);

    CHECK(emulator.pc == 12);
    CHECK(emulator.I == 100);
    CHECK(emulator.memory[100] == 2);
    CHECK(emulator.memory[101] == 5);
    CHECK(emulator.memory[102] == 4);
  }

  SUBCASE(
      "FX55 should store V[0] to V[X] in memory starting at address I and increment program "
      "counter counter by 2") {
    emulator.pc = 10;
    emulator.I = 100;
    emulator.V[0] = 255;
    emulator.V[1] = 10;
    emulator.V[2] = 20;
    emulator.V[3] = 30;
    emulator.V[4] = 40;
    emulator.V[5] = 50;

    emulator.execute_opcode(0xF555);

    CHECK(emulator.pc == 12);
    CHECK(emulator.I == 100);
    CHECK(emulator.memory[100] == 255);
    CHECK(emulator.memory[101] == 10);
    CHECK(emulator.memory[102] == 20);
    CHECK(emulator.memory[103] == 30);
    CHECK(emulator.memory[104] == 40);
    CHECK(emulator.memory[105] == 50);
  }

  SUBCASE(
      "FX65 should fill V[0] to V[X] with values from memory starting at address I and increment "
      "program counter counter by 2") {
    emulator.pc = 10;
    emulator.I = 100;

    emulator.V[0] = 0;
    emulator.V[1] = 0;
    emulator.V[2] = 0;
    emulator.V[3] = 0;
    emulator.V[4] = 0;
    emulator.V[5] = 0;

    emulator.memory[100] = 255;
    emulator.memory[101] = 10;
    emulator.memory[102] = 20;
    emulator.memory[103] = 30;
    emulator.memory[104] = 40;
    emulator.memory[105] = 50;

    emulator.execute_opcode(0xF565);

    CHECK(emulator.pc == 12);
    CHECK(emulator.I == 100);
    CHECK(emulator.V[0] == 255);
    CHECK(emulator.V[1] == 10);
    CHECK(emulator.V[2] == 20);
    CHECK(emulator.V[3] == 30);
    CHECK(emulator.V[4] == 40);
    CHECK(emulator.V[5] == 50);
  }
}

TEST_CASE("Emulator has flags") {
  EmulatorTest emulator;

  SUBCASE("The draw flag should be set to false after checking it") {
    emulator.draw_flag = true;

    CHECK(emulator.should_draw());
    CHECK(!emulator.should_draw());
  }

  SUBCASE("The sound flag should be set to false after checking it") {
    emulator.sound_flag = true;

    CHECK(emulator.should_buzz());
    CHECK(!emulator.should_buzz());
  }
}
