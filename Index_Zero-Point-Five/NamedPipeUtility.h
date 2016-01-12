/*
    I never finish anythi

    Author: Joey Cluett

    Date Created: 12/19/2015

    Date Last Modified: 1/3/2016

    Purpose:
        This library serves to make the usage of simplex and half-duplex
        POSIX-standard named pipes easier.
        WARNING: This library is NOT cross-platform. It makes several
            Linux specific system calls and so will not work on Windows.
            It may work on Mac OS.

    Function List:
        Simplex_create();
        Simplex_open();
        Simplex_write();
        Simplex_read();
        Simplex_close();
        Simplex_destroy();

        //
            a half duplex named-pipe is really just two simplex pipes
            this library presents it as a single entity using simple read-/write-duplex functions

        HalfDuplex_create();
        HalfDuplex_open_rx_first();
        HalfDuplex_open_tx_first();
        HalfDuplex_write();
        HalfDuplex_read();
        HalfDuplex_close();
        HalfDuplex_destroy();

    Misc. Information:
        Initial Compile on Ubuntu 14.04 Trusty Tahr with GCC 4.8.4
        Pipe names must be absolute. Ex.: "/tmp/pipe_1" or "/tmp/pipe_2"
        It is very good programming practice to put all named pipes in /tmp (temporary)
        Everything needed for a fully working pipe:
            1.) pipename
            2.) buffer (char array preferrably)
            3.) buffer size (in DataType, use sizeof() if you are unsure)
        When opening a full-duplex pipe:
            Use HalfDuplex_open_method_1() in prog1
            Use HalfDuplex_open_method_2() in prog2
            This is to avoid problems with both programs blocking until the other opens
*/

#ifndef NAMEDPIPEUTILITY_H
#define NAMEDPIPEUTILITY_H

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define IS_WRITER 1
#define IS_READER 2

class NamedPipeUtility {
    public:

    //------------------------------------------------------------------------------------

        NamedPipeUtility(char buffer[], int buffer_capacity); //simplex operation
        NamedPipeUtility(char rx_buf[], int rx_buffer_capacity, char tx_buf[], int tx_buffer_capacity); //half-duplex operation
        NamedPipeUtility(void);

    //------------------------------------------------------------------------------------

        void Simplex_create(char* pipename, bool create);
        int  Simplex_open(int r_wr_flag); //1 = writer, 2 = reader

        //reading and writing to/from a simplex pipe
        void Simplex_write(char buffer[], int buffer_size); //user supplies all information neccessary for writing to pipe
        void Simplex_write(char buffer[]); //finds size of used buffer using private method
        void Simplex_write(void); //uses tx_buffer for writing information
        void Simplex_write(unsigned char character); //useful for writing literal data

        void Simplex_read(char buffer[], int buffer_size);
        void Simplex_read(void); //uses rx_buffer for reading information
        void Simplex_read(unsigned char* character); //useful for reading literal data

        void Simplex_close(void);
        void Simplex_destroy(void);

    //------------------------------------------------------------------------------------

        void HalfDuplex_create(char* tx_pipename, char* rx_pipename, bool create); //both programs must have pipenames but only one needs to create them
        void HalfDuplex_open_method_1(void); //Pipe operations are automatically assigned
        void HalfDuplex_open_method_2(void); //Pipe operations are automatically assigned

        //reading and writing to/from a full duplex pipe
        void HalfDuplex_write(char txbuffer[], int txbuffer_size);
        void HalfDuplex_write(char txbuffer[]); //finds size of used buffer using private method
        void HalfDuplex_write(void); //uses char* tx_buffer and tx_buffer_size

        void HalfDuplex_read(char rxbuffer[], int rxbuffer_size);
        void HalfDuplex_read(void); //uses char* rx_buffer and rx_buffer_size

        void HalfDuplex_close(void);
        void HalfDuplex_destroy(void);

    //------------------------------------------------------------------------------------

        //misc. methods
        void ClearBuffer(char buffer[], int buffer_size); //clear entire buffer
        void ClearBuffer(char buffer[]); //use only when you are certain the buffer isnt being 100% used

    //------------------------------------------------------------------------------------

    private:
        int retUsedSize(char buf[]); //expects a null terminated array...

        char* rx_pipe; //receive pipe name
        char* tx_pipe; //transmit pipe name

        char* tx_buffer; //gives NamedPipeUtility a handle on the transmit buffer
        char* rx_buffer; //gives NamedPipeUtility a handle on the receive buffer

        int tx_buffer_size, rx_buffer_size; //these numbers reflect the capacity of the transmit/receive buffers

        int fd_rx;
        int fd_tx;


};

#endif // NAMEDPIPEUTILITY_H
