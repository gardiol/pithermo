
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
        if ( fread( &a, sizeof(a), 1, f ) == 1 )
			if ( fread( &b, sizeof(b), 1, f ) == 1 )
				if ( fread( &c, sizeof(c), 1, f ) == 1 )
                    if ( fread( &d, sizeof(d), 1, f ) == 1 )
                    {
                        uint64_t a_be = le64toh( a );
                        uint32_t b_be = le32toh( b );
                        uint32_t c_be = le32toh( c );
                        uint32_t d_be = le32toh( d );
                        fwrite( &a_be, sizeof(a_be), 1, t );
                        fwrite( &b_be, sizeof(b_be), 1, t );
                        fwrite( &c_be, sizeof(c_be), 1, t );
                        fwrite( &d_be, sizeof(d_be), 1, t );
                        printf("(%d) le: %lu / %u / %u / %u = %lu / %u / %u / %u\n", count, a, b, c, d, a_be, b_be, c_be , d_be);
                        count++;
                    }
	}
	fclose(f);
	fclose(t);

	return 0;
}
