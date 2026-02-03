/*
Copyright 2026 Spalishe

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

*/

#include <iostream>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <csignal>
#include "../include/devices/uart.hpp"

termios orig_termios;

std::vector<char> combo_sequence = {3, 24}; // Ctrl+C (0x03) -> Ctrl+X (0x18)
std::vector<char> buffer;

void restore_terminal(int) {
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
    _exit(0);
}

void termios_start() {
    tcgetattr(STDIN_FILENO, &orig_termios);

    signal(SIGINT, restore_terminal);   // Ctrl+C

    atexit([](){ tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios); });

    termios new_termios = orig_termios;
    new_termios.c_lflag &= ~(ICANON | ECHO | ISIG);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);

    fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL, 0) | O_NONBLOCK);
}

void termios_loop(UART* uart,Machine& machine) {
    char c;
    if (read(STDIN_FILENO, &c, 1) > 0) {
        buffer.push_back(c);

        if (buffer.size() > combo_sequence.size()) {
            buffer.erase(buffer.begin());
        }

        if (buffer.size() == combo_sequence.size() && buffer == combo_sequence) {
            machine_poweroff(machine);
            buffer.clear();
            return;
        }

        uart->receive_byte(c);
    }
}