#ifndef CONSOLE_H
    #define CONSOLE_H

    #ifdef __cplusplus
        extern "C" {
    #endif



    void console_init( void );
    /*
    void console_print( const char * fmt,
                        ... );
                        */

    void console_print(const char* fmt,
        ...);

    void write_output_to_file(void);

    #ifdef __cplusplus
        }
    #endif

#endif /* CONSOLE_H */
