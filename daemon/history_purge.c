
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
        uint64_t ti;
        float te;
        float ete;
        float ehu;
	float hu;
        if ( fread( &ti, sizeof(ti), 1, f ) == 1 )
            if ( fread( &te, sizeof(te), 1, f ) == 1 )
                if ( fread( &ete, sizeof(ete), 1, f ) == 1 )
                    if ( fread( &ehu, sizeof(ehu), 1, f ) == 1 )
                    	if ( fread( &hu, sizeof(hu), 1, f ) == 1 )
                    	{
				int skip = ( (te < -50.0f) ||
				             (te > 50.0f) ||
				             (ete < -50.0f) ||
				             (ete > 50.0f) ||
				             (ehu < -100.0f) ||
				             (ehu > 100.0f) ||
				             (hu < -100.0f) ||
				             (hu > 100.0f) );
				if ( !skip )
				{
                        		fwrite( &ti, sizeof(ti), 1, t );
                        		fwrite( &te, sizeof(te), 1, t );
                        		fwrite( &ete, sizeof(ete), 1, t );
                        		fwrite( &ehu, sizeof(ehu), 1, t );
                        		fwrite( &hu, sizeof(hu), 1, t );
				}
				else
                        		count++;
                    	}
    }
    fclose(f);
    fclose(t);

	printf("Skipped: %d\n", count );
    return 0;
}
