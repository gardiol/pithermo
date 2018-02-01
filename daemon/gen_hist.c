
#include <endian.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

int main(int argc, char** argv)
{
	if ( argc < 2 )
	{
		printf("Use: %s hist\n", argv[0] );
		return 1;
	}

	FILE* hf = fopen( argv[1], "w" );

	uint64_t t = time(NULL) - 3600*24*365;
	float te = 11.0;
	float hu = 20.0;
	for ( uint32_t x = 0; x < 60*24*365; ++x )
	{
		fwrite( &t, sizeof(t), 1, hf );
		fwrite( &te, sizeof(te), 1, hf );
		fwrite( &hu, sizeof(hu), 1, hf );
		printf("(%d) %lu / %f / %f\n", x, t, te, hu );
		t+=60;
		te += 0.1;
		hu += 0.5;
		if ( te > 35.0 )
			te = 11.0;
		if ( hu > 90 )
			hu = 20.0;
	}
	fclose(hf);

	return 0;
}
