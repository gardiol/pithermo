
#include <endian.h>
#include <stdint.h>
#include <stdio.h>

int main(int argc, char** argv)
{
    if ( argc < 3 )
    {
        printf("Use: %s from to\n", argv[0] );
        return 1;
    }

    FILE* f = fopen( argv[1], "r" );
    FILE* t = fopen( argv[2], "w" );

    uint32_t count = 0;
    while ( !feof(f) )
    {
        uint64_t a;
        uint32_t b;
        uint32_t c;
        uint32_t d;
        uint32_t x = 0;
        if ( fread( &a, sizeof(a), 1, f ) == 1 )
            if ( fread( &b, sizeof(b), 1, f ) == 1 )
                if ( fread( &c, sizeof(c), 1, f ) == 1 )
                    if ( fread( &d, sizeof(d), 1, f ) == 1 )
                    {
                        fwrite( &a, sizeof(a), 1, t );
                        fwrite( &b, sizeof(b), 1, t );
                        fwrite( &c, sizeof(c), 1, t );
                        fwrite( &d, sizeof(d), 1, t );
                        fwrite( &x, sizeof(x), 1, t );
                        count++;
                    }
    }
    fclose(f);
    fclose(t);

    return 0;
}
