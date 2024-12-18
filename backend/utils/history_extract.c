
#include <endian.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include "linux/kd.h"
#include "termios.h"
#include "fcntl.h"
#include <sys/ioctl.h>
#include <pwd.h>
#include <stdlib.h>

int create_path( char* path, uint32_t w, uint32_t d )
{
    char tmp[1024];
    strcat( path, "/" );
    sprintf( tmp, "%d", w );
    strcat( path, tmp );
    mkdir( path, 0750 );
    strcat( path, "/" );
    sprintf( tmp, "%d", d );
    strcat( path, tmp );
    mkdir( path, 0750 );
    strcat( path, "/" );
    strcat( path, "history_data" );
    return 1;
}

int check_epoc( const char* filename, uint64_t epoc )
{
    int ret = 1;
    FILE* file = fopen( filename, "r" );
    if ( file != NULL )
    {
        while ( !feof(file) && (ret == 1) )
        {
            uint64_t t;
            float a,b,c,d;
            fscanf( file, "%llu %f %f %f %f\n", (unsigned long long int*)&t, &a, &b, &c, &d);
            if ( t == epoc )
                ret = 0;
        }
        fclose(file);
    }
    return ret;
}

int main(int argc, char** argv)
{

    printf("History Extractor tool\n");
    if ( argc < 3 )
    {
        printf("Use: %s <from_file> <to_path>\n", argv[0] );
        return 1;
    }

    printf("Opening file %s and populate folders in %s...\n", argv[1], argv[2] );
    FILE* src_file = fopen( argv[1], "rb" );
    if ( src_file == NULL )
    {
        printf("Unable to open source file\n");
        return 3;
    }

    if ( access( argv[2], R_OK) != 0 )
    {
        printf("Destination folder invalid\n");
        return 4;
    }

    while ( !feof( src_file ) )
    {
        // Read data
        uint64_t epoc;
        float temp;
        float ext_temp;
        float ext_humi;
        float humi;

        if ( fread( &epoc, sizeof(epoc), 1, src_file ) == 1 )
        {
            if ( fread( &temp, sizeof(temp), 1, src_file ) == 1 )
            {
                if ( fread( &ext_temp, sizeof(ext_temp), 1, src_file ) == 1 )
                {
                    if ( fread( &ext_humi, sizeof(ext_humi), 1, src_file ) == 1 )
                    {
                        if ( fread( &humi, sizeof(humi), 1, src_file ) == 1 )
                        {
                            // Normalize start day: for EPOC is thursday, for us it's monday.
                            // what we do is to "add" three days to the time, so that:
                            // - EPOC day0 becomes a monday instead of a thursday
                            // - EPOC day3 becomes a thursday, correctly.
                            // Note that this will also shift WEEKS! Now the first EPOC week
                            // will be 1 and not 0. This means that the "w" below is shifted by one...
                            // but we don't care for that since we always work for "difference" between
                            // week numbers.
                            uint64_t normalized_now = epoc + 60*60*24*3; // Now the day-0 for EPOC=3 will be +3days
                            uint32_t day = (normalized_now / (60*60*24)) % 7; // Days are 0..6 (mon-sun)
                            uint32_t week  = ((normalized_now / (60*60*24*7))); // weeks are not bounded..
                            char filename[1024];
                            strcpy( filename, argv[2] );
                            create_path( filename, week, day );
                            if ( check_epoc( filename, epoc ) )
                            {
                                FILE* output_file = fopen( filename, "a" );
                                if ( output_file != NULL )
                                {
                                    fprintf( output_file, "%llu %f %f %f %f\n", (unsigned long long int)epoc, (double)temp, (double)ext_temp, (double)humi, (double)ext_humi);
                                    fclose(output_file);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    fclose(src_file);
    return 0;
}
