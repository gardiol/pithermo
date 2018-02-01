
#include <endian.h>
#include <stdint.h>
#include <stdio.h>

int main(int argc, char** argv)
{
	if ( argc < 2 )
	{
		printf("Use: %s hist\n", argv[0] );
		return 1;
	}

	FILE* f = fopen( argv[1], "r" );

	uint32_t count = 0;
	while ( !feof(f) )
	{
		uint64_t a;
		float b;
		float c;
		if ( fread( &a, sizeof(a), 1, f ) == 1 )
			if ( fread( &b, sizeof(b), 1, f ) == 1 )
				if ( fread( &c, sizeof(c), 1, f ) == 1 )
				{
					printf("(%d) %lu / %f / %f\n", count, a, b, c  );
					count++;
				}
	}
	fclose(f);

	return 0;
}
