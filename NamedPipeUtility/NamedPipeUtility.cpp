/*
    Implementation file for NamedPipeUtility

    Author: Joey Cluett

    Date Created: 12/20/2015

    Date Last Modified: 1/3/2016

    Purpose:
        This library serves to make the usage of simplex and half-duplex
        POSIX-standard named pipes easier.
*/

#include <iostream>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "NamedPipeUtility.h"

using namespace std;

// Constructors -----------------------------------------------------------------------------------

NamedPipeUtility::NamedPipeUtility(char buffer[], int buffer_capacity) {
    tx_buffer = buffer;
    tx_buffer_size = buffer_capacity;
}

NamedPipeUtility::NamedPipeUtility(char rx_buf[], int rx_buffer_capacity, char tx_buf[], int tx_buffer_capacity) {
    //assign receive buffer
    rx_buffer = rx_buf;
    rx_buffer_size = rx_buffer_capacity;

    //assign transmit buffer
    tx_buffer = tx_buf;
    tx_buffer_size = tx_buffer_capacity;
}

NamedPipeUtility::NamedPipeUtility(void) {
    cout << "Default buffers not assigned. Communication" << endl << "may not perform as expected" << endl;
}

//-------------------------------------------------------------------------------------------------

// Simplex methods ----------------------------------------------------------------------------

void NamedPipeUtility::Simplex_create(char* pipename, bool create) {
    tx_pipe = pipename;
    if(create){
        mkfifo(tx_pipe, 0666);
    }
}

int NamedPipeUtility::Simplex_open(int r_wr_flag) {
    if(r_wr_flag == 1) {
        //open pipe as writer
        fd_tx = open(tx_pipe, O_WRONLY);
        return 0;
    } else if(r_wr_flag == 2) {
        //open pipe as reader
        fd_tx = open(tx_pipe, O_RDONLY);
        return 0;
    } else {
        cout << "Valid communication method not given" << endl;
        return -1;
    }
}

void NamedPipeUtility::Simplex_write(char buffer[], int buffer_size) {
    write(fd_tx, buffer, buffer_size);
}

void NamedPipeUtility::Simplex_write(char buffer[]) { //finds used size
    int usedSize = retUsedSize(buffer);
    write(fd_tx, buffer, usedSize);
}

void NamedPipeUtility::Simplex_write(void) {
    write(fd_tx, tx_buffer, tx_buffer_size);
}

void NamedPipeUtility::Simplex_write(unsigned char character) {
    write(fd_tx, &character, 1);
}

void NamedPipeUtility::Simplex_read(char buffer[], int buffer_size) {
    read(fd_tx, buffer, buffer_size);
}

void NamedPipeUtility::Simplex_read(void) {
    read(fd_tx, tx_buffer, tx_buffer_size);
}

void NamedPipeUtility::Simplex_read(unsigned char* character) {
    read(fd_tx, character, 1);
}

void NamedPipeUtility::Simplex_close(void) {
    close(fd_tx);
}

void NamedPipeUtility::Simplex_destroy(void) {
    unlink(tx_pipe);
}

//-------------------------------------------------------------------------------------------------

// Half-Duplex methods ----------------------------------------------------------------------------

void NamedPipeUtility::HalfDuplex_create(char* tx_pipename, char* rx_pipename, bool create) {
    //assign pipe names to local class variables
    tx_pipe = tx_pipename;
    rx_pipe = rx_pipename;

    //create two pipes if neccessary
    if(create) {
        mkfifo(tx_pipe, 0666);
        mkfifo(rx_pipe, 0666);
    }
}

void NamedPipeUtility::HalfDuplex_open_method_1(void) {
    //open first pipe as writer then open second pipe as reader
    fd_tx = open(tx_pipe, O_WRONLY);
    fd_rx = open(rx_pipe, O_RDONLY);
}

void NamedPipeUtility::HalfDuplex_open_method_2(void) {
    //open first pipe as reader then open second pipe as writer
    fd_rx = open(rx_pipe, O_RDONLY);
    fd_tx = open(tx_pipe, O_WRONLY);
}

void NamedPipeUtility::HalfDuplex_write(char txbuffer[], int txbuffer_size) {
    write(fd_tx, txbuffer, txbuffer_size);
}

void NamedPipeUtility::HalfDuplex_write(char txbuffer[]) {
    int usedSize = retUsedSize(txbuffer);
    write(fd_tx, txbuffer, usedSize);
}

void NamedPipeUtility::HalfDuplex_write(void) {
    write(fd_tx, tx_buffer, tx_buffer_size);
}

void NamedPipeUtility::HalfDuplex_read(char rxbuffer[], int rxbuffer_size) {
    read(fd_rx, rxbuffer, rxbuffer_size);
}

void NamedPipeUtility::HalfDuplex_read(void) {
    read(fd_rx, rx_buffer, rx_buffer_size);
}

void NamedPipeUtility::HalfDuplex_close(void) {
    close(fd_tx);
    close(fd_rx);
}

void NamedPipeUtility::HalfDuplex_destroy(void) {
    unlink(rx_pipe);
    unlink(tx_pipe);
}

//-------------------------------------------------------------------------------------------------

// Miscellaneous methods --------------------------------------------------------------------------

void NamedPipeUtility::ClearBuffer(char buffer[], int buffer_size) {
    for(int i = 0; i < buffer_size; i++) {
        buffer[i] = 0;
    }
}

void NamedPipeUtility::ClearBuffer(char buffer[]) {
    for(int i = 0; buffer[i] != 0; i++) {
        buffer[i] = 0;
    }
}

int NamedPipeUtility::retUsedSize(char buf[]) { //expects a null-terminated array
    int used = 0;
    for(int i = 0; buf[i] != 0; i++) {
        used++;
    }
    return used;
}

//-------------------------------------------------------------------------------------------------


//Beware of bugs in the above code. I have only proved it correct, not tested it.

