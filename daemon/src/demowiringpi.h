#ifndef DEMOWIRINGPI_H
#define DEMOWIRINGPI_H

extern int  wiringPiSetup       (void) ;

extern void pinMode             (int pin, int mode) ;
extern int  digitalRead         (int pin) ;
extern void digitalWrite        (int pin, int value) ;
extern void         delay             (unsigned int howLong) ;
extern void         delayMicroseconds (unsigned int howLong) ;

#define	LOW			 0
#define	HIGH			 1

#define	INPUT			 0
#define	OUTPUT			 1

#endif // DEMOWIRINGPI_H
