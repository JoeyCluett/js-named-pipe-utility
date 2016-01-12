/*
    Author: Joey Cluett

    Date Created: 1/2/2016

    Date Last Modified: 1/5/2016

    Purpose:
        Interpreter for Index 0.5 (Homebrew Computer v3) aka 'Index Emulator' or 'Index Virtual Machine'
*/

#include <iostream>
#include <fstream>
#include <vector>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include "NamedPipeUtility.h"
#include "keyboard.h"

using namespace std;

NamedPipeUtility keyboardreader;

int address_space = 0xFFFF; //default 16-bit address space

vector<unsigned char> RAM; //virtual RAM

bool halted = false;
bool debug = false;
bool step = false;
bool screen = false;
bool keyboard = false;

int accumulator = 0; //results of alu operations are stored here
int PC = 0; //execution always starts at index zero, programs up to 2GB can be used
int SP = 0; //stack pointer, initially set to 0
int address_stack[16]; //for function calls
int ASP = 0; //address stack pointer

unsigned char reg[8]; //8 GP registers
unsigned char reg_a = 0, reg_b = 0; //ALU registers
unsigned char flag_reg = 0; //for storing ALU flag, only LS 4 bits are used
unsigned char keyboard_reg = 0;

int check_arguments(char* argument, int argc, char* argv[]) {
    for(int i = 1; i < argc; i++) {
        if(!strcmp(argv[i], argument))
            return 1;
    }
    return 0;
}

//---------------------------------------------------------------------------------------

/*Done*/ void load_alu_reg(char opcode_traits) { //0x0
    if(debug)
        cout << "Load ALU Register(s)" << endl;

    if(opcode_traits & 0x08 == 0) {
        //register A
        reg_a = reg[opcode_traits & 0x07];
    } else {
        //register B
        reg_b = reg[opcode_traits & 0x07];
    }

    PC += 1;
}

void save_acc(char opcode_traits) { //0x1
    if(debug)
        cout << "Save Accumulator" << endl;
    PC += 1;
}

void ld_sv_reg(char opcode_traits, unsigned char MSB, unsigned char LSB) { //0x2
    if(debug)
        cout << "Load/Save Register Operation" << endl;
    PC += 3;
}

void alu_op(char operation) { //0x3
    if(debug)
        cout << "ALU Operation" << endl;
    PC += 1;
}

void stack_op(char opcode_traits) { //0x4
    if(debug)
        cout << "Stack Operation" << endl;
    PC += 1;
}

/*Done*/ void absolute_jump(unsigned char MSB, unsigned char LSB) { //0x5
    if(debug)
        cout << "Absolute Jump" << endl;

    //cout << "MSB: " << (int) MSB << endl << "LSB: " << (int) LSB << endl;

    PC = 0;
    PC = MSB;
    PC = PC << 8;
    PC = PC | LSB;
}

void conditional_jump(char opcode_traits, unsigned char relative_jump_distance) { //0x6
    if(debug)
        cout << "Conditional Jump" << endl;
    PC += 2;
}

void move_bytes(char opcode_traits, unsigned char MSB, unsigned char LSB) { //0x7
    if(debug)
        cout << "Move Bytes" << endl;
    PC += 3;
}

/*Done*/ void set_stack_pointer(unsigned char MSB, unsigned char LSB) { //0x8
    if(debug)
        cout << "Set Stack Pointer" << endl;

    //modify stack pointer
    SP = 0;
    SP = (MSB << 8);
    SP = SP | LSB;

    PC += 3;
}

/*Done*/ void jump_to_subroutine(unsigned char MSB, unsigned char LSB) { //0x9
    if(debug)
        cout << "Jump to Subroutine" << endl;

    //push PC onto address stack
    if(debug) {
        if(ASP >= 16) {
            cout << "Error with address stack pointer: overflow" << endl;
        } else if(ASP < 0) {
            cout << "Error with address stack pointer: underflow" << endl;
        }
    }

    address_stack[ASP % 16] = PC;
    ASP++;

    PC = 0;
    PC = MSB;
    PC = PC << 8;
    PC = PC | LSB;

    if(debug)
        cout << "Address Stack Pointer: " << ASP << endl;
}

/*Done*/ void return_from_subroutine(void) { //0xA
    if(debug)
        cout << "Return from Subroutine" << endl;

    //pop PC off of address stack
    ASP--;
    PC = reg[ASP % 16];

    if(debug) {
        if(ASP >= 16) {
            cout << "Error with address stack pointer: overflow" << endl;
        } else if(ASP < 0) {
            cout << "Error with address stack pointer: underflow" << endl;
        }
    }

    if(debug)
        cout << "Address Stack Pointer: " << ASP << endl;
}

//0xB -- GPIO Ops

//0xC -- GPIO Ops

//0xD -- Not Currently Defined

//0xE -- Not Currently Defined

void special_cpu_function(char opcode_traits) { //0xF
    if(debug)
        cout << "Special CPU Function: ";
    switch(opcode_traits) {
        case 0: //halt
            if(debug)
                cout << "halt" << endl;
            halted = true;
            break;
        case 1: //write next 8 bytes into GP registers
            if(debug)
                cout << "nextr" << endl;
            PC += 9;
            break;
        case 2: //write GP registers into next 8 memory locations
            if(debug)
                cout << "nextm" << endl;
            PC += 9;
            break;
        case 3: //nop
            if(debug)
                cout << "nop" << endl;
            PC += 1;
            break;
        case 4: //write bytes to screen
            if(debug)
                cout << "wscr" << endl;
            PC += 3;
            break;
        case 5: //read keyboard input
            if(debug)
                cout << "rdkey" << endl;
            PC += 2;
            break;
        case 6: //reset keyboard input register
            if(debug)
                cout << "rskey" << endl;
            keyboard_reg = 0;
            PC += 1;
            break;
        default:
            cout << "Operation not defined" << endl;
            halted = true;
    }
}

//---------------------------------------------------------------------------------------

/*THIS FUNCTION IS DEPRECATED AND NO LONGER USED*/
//thread that gets forked to control interpreter/keyboard communication
void* start_keyboard(void* datum) {
    system("./x4_keyboard");
    while(1) {
        ; //do nothing if x4_keyboard quits early
    }
}

//---------------------------------------------------------------------------------------

int main(int argc, char* argv[]) {

    if(argc < 2) {
        cout << "usage: index <hex file name> <debug=true/false> <step=true/false> <screen=true/false> <keyboard=true/false>" << endl;
        cout << "The last 4 arguments are order agnostic" << endl;
        return 1;

    } else {

        /* set/reset debug variable */
        int sentinal = 0;
        debug = check_arguments("debug=true", argc, argv);
        sentinal += debug;
        sentinal += check_arguments("debug=false", argc, argv);
        //exit if no debug status has been set
        if(sentinal == 0) {
            cout << "no debug status set. exiting interpreter..." << endl; return 1;
        }

        /* set/reset step variable */
        sentinal = 0;
        step = check_arguments("step=true", argc, argv);
        sentinal += step;
        sentinal += check_arguments("step=false", argc, argv);
        //exit if no step variable has been set
        if(sentinal == 0) {
            cout << "no step status set. exiting interpreter..." << endl; return 1;
        }

        /* set/reset screen variable */
        sentinal = 0;
        screen = check_arguments("screen=true", argc, argv);
        sentinal += screen;
        sentinal += check_arguments("screen=false", argc, argv);
        //exit if no screen variable has been set
        if(sentinal == 0) {
            cout << "no screen status set. exiting interpreter..." << endl; return -1;
        }

        /* set/reset keyboard variable */
        sentinal = 0;
        keyboard = check_arguments("keyboard=true", argc, argv);
        sentinal += keyboard;
        sentinal += check_arguments("keyboard=false", argc, argv);
        //exit if no screen variable has been set
        if(sentinal == 0) {
            cout << "no keyboard status set. exiting interpreter..." << endl; return -1;
        }

        cout << "Debug value: " << debug << endl << "Step value: " << step << endl;
        cout << "Screen value: " << screen << endl << "Keyboard value: " << keyboard << endl;

        /* setup communications between interpreter and keyboard process */
        if(keyboard) {
            pthread_t keyboard_thread;
            pthread_create(&keyboard_thread, NULL, keyboard_fork, NULL); //keyboard_fork is defined in keyboard.h
            keyboardreader.Simplex_create("/tmp/keyboardnpu", false);
            usleep(2000000);
            keyboardreader.Simplex_open(2); //open as reader
        }

        if(step)
            cout << "Press ENTER to step through the program" << endl << "Type double backslash '\\\\' then ENTER to exit" << endl;

        /* Load hex file into interpreter */
        ifstream hexfile(argv[1], ifstream::in);
        unsigned char c = hexfile.get();
        while(hexfile.good()) { //while file has unread data
            RAM.push_back(c);
            c = hexfile.get();
        }

        if(RAM.size() > 0xFFFF) { //check size of memory taken by program
            cout << "Size of program has exceeded memory of physical device. Continue (y/n)?? ";
            cin >> c;
            if(c == 'n') {
                return 1;
            } else { //if user wants to run abnormally sized program, they can
                address_space = RAM.size(); //update address space variable
            }
        } else { //fill remaining space with 0's, if program requires less than 16-bit address space
            for(int i = RAM.size(); i <= 0xFFFF; i++) {
                RAM.push_back(0xF2);
            }
        }

        cout << "RAM size: " << RAM.size() << endl << endl; //useful information for user to know

        //set GP registers to default value
        for(int i = 0; i < 8; i++) {
            reg[i] = 0;
        }

        //set address stack regsiters to default value
        for(int i = 0; i < 16; i++) {
            address_stack[i] = 0;
        }

    }

    //hex file is now loaded or interpreter has quit

    unsigned char inst_reg, opcode;

    unsigned int inst_count = 0; //for 'abnormal' debugging purposes, will track up to 4 billion-ish instructions
    unsigned char keyread;

    keyboard_reg = 0;

    while(!halted) {
        inst_reg = RAM[PC % address_space]; //to prevent overflow
        opcode = inst_reg & 0xF0;
        switch(opcode >> 4) { //switch just the opcode
            case  0: //0x0
                load_alu_reg(inst_reg & 0x0F);
                break;
            case  1: //0x1
                save_acc(inst_reg & 0x0F);
                break;
            case  2: //0x2
                ld_sv_reg(inst_reg & 0x0F, RAM[(PC + 1) % address_space], RAM[(PC + 2) % address_space]);
                break;
            case  3: //0x3
                alu_op(inst_reg & 0x0F);
                break;
            case  4: //0x4
                stack_op(inst_reg & 0x0F);
                break;
            case  5: //0x5
                absolute_jump(RAM[(PC + 1) % address_space], RAM[(PC + 2) % address_space]);
                break;
            case  6: //0x6
                conditional_jump(inst_reg & 0x0F, RAM[(PC + 1) % address_space]);
                break;
            case  7: //0x7
                move_bytes(inst_reg & 0x0F, RAM[(PC + 1) % address_space], RAM[(PC + 2) % address_space]);
                break;
            case  8: //0x8
                set_stack_pointer(RAM[(PC + 1) % address_space], RAM[(PC + 2) % address_space]);
                break;
            case  9: //0x9
                jump_to_subroutine(RAM[(PC + 1) % address_space], RAM[(PC + 2) % address_space]);
                break;
            case 10: //0xA
                return_from_subroutine();
                break;
            case 11: //0xB
                cout << "GPIO Instruction Not Currently Defined" << endl;
                return 1; //Intepreter quits execution w/ error
            case 12: //0xC
                cout << "GPIO Instruction Not Currently Defined" << endl;
                return 1; //Intepreter quits execution w/ error
            case 13: //0xD
                cout << "Error: Instruction Not Defined" << endl;
                return 1; //Interpreter quits execution w/ error
            case 14: //0xE
                cout << "Error: Instruction Not Defined" << endl;
                return 1; //Interpreter quits execution w/ error
            case 15: //0xF
                special_cpu_function(inst_reg & 0x0F);
                break;
            default:
                cout << "Error: Instruction Not Defined" << endl;
                return 1; //Interpreter quits execution w/ error
        }

        //optionally display the value of the program counter and instruction counter after each instruction
        if(debug) {
            cout << "Program Counter: " << PC << endl;
            inst_count++;
            cout << "Inst count: " << inst_count << endl;
        }

        //optionally step through each instruction
        if(step) {
            char c = getchar();
            while(c != '\n') {
                c = getchar();
                if(c == 92) { //test for backslash
                    halted = true;
                }
            }
        }

        //optionally update keyboard register
        if(keyboard) {
            keyboardreader.Simplex_read(&keyread);
            if(keyread != -1) {
                keyboard_reg = keyboard_reg | keyread;
                cout << "Keyboard register: " << (int) keyboard_reg << endl;
            }
        }

        //----------------------End of main loop
    }
}

//Beware of bugs in the above code. I have only proved it correct, not tested it. compiled != finished
