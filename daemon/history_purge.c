
#include <endian.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>

// check for "zero" values and filter them out
int zero_check( float* cur, float prev, float step )
{
    int ret = 0;
    // cur is 0, but it's a big step from prev:
    if ( ( fabsf(*cur) < 0.01f ) && (fabsf(prev) > step) )
    {
        *cur = prev;
        ret = 1;
    }
    return ret;
}

// Check for exagerate jumps in values
int jump_check( float* cur, float prev, float step )
{
    int ret = 0;
    // The difference between values is bigger than step:
    if ( fabsf(*cur - prev) > step )
    {
        *cur = prev;
        ret = 1;
    }
    return ret;
}

int main(int argc, char** argv)
{
    int (*checks[2])(float*,float,float) = { zero_check, jump_check };
    char checks_descr[2][50] = { "Zero", "Jump" };

    printf("History fixer and cleaner tool\n");
    if ( argc < 3 )
    {
        printf("Use: %s <from> <to>\n", argv[0] );
        return 1;
    }
    if ( strcmp( argv[1], argv[2] ) == 0 )
    {
        printf("Unable to read and write the same file.\n");
        return 2;
    }

    printf("Opening file %s and writing to %s...\n", argv[1], argv[2] );
    FILE* src_file = fopen( argv[1], "rb" );
    if ( src_file == NULL )
    {
        printf("Unable to open source file\n");
        return 3;
    }
    FILE* dst_file = fopen( argv[2], "wb" );
    if ( dst_file == NULL )
    {
        printf("Unable to open destination file\n");
        return 4;
    }

    FILE* plot_file = fopen( "history.dat", "w" );
    if ( plot_file == NULL )
    {
        printf("Unable to open \"history.dat\" plot data file\n");
        return 4;
    }
    fprintf(plot_file, "#epoc temp ext_temp humi ext_humi\n");

    uint32_t total_count = 0;
    uint32_t read_count = 0;
    uint32_t write_count = 0;
    uint32_t edit_count = 0;
    uint32_t invalid_count = 0;

    uint64_t prev_epoc;
    float prev_temp;
    float prev_ext_temp;
    float prev_ext_humi;
    float prev_humi;

    uint64_t now_epoc = (uint64_t)time(NULL);

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
                            // Read one full record!
                            // Is valid?
                            if ( ((epoc > 1483232461) &&
                                  (epoc <= now_epoc)) &&
                                 ((temp > -60.0f) &&
                                  (temp < 60.0f)) &&
                                 ((ext_temp > -60.0f) &&
                                  (ext_temp < 60.0f)) &&
                                 ((ext_humi >= 0.0f) &&
                                  (ext_humi <= 100.0f)) &&
                                 ((humi >= 0.0f) &&
                                  (humi <= 100.0f)) &&
                                 // Ignore non progressive epocs
                                 ((read_count == 0) || ((read_count > 0) && (prev_epoc < epoc)) ) )
                            {
                                // Record is VALID!

                                // Check inusual "steps"
                                if ( read_count > 0 )
                                {
                                    int edit = 0;
                                    char val_descr[4][50] = { "Temp", "ExtTemp", "Humi", "ExtHumi" };
                                    float* val[4] = { &temp, &ext_temp, &humi, &ext_humi };
                                    float prev_val[4] = { prev_temp, prev_ext_temp, prev_humi, prev_ext_humi };
                                    float step[4][2] = { {5.0f,2.0f},
                                                         {5.0f,2.0f},
                                                         {5.0f,25.0f},
                                                         {5.0f,25.0f} };
                                    for ( int v = 0; v < 4; v++ )
                                    {
                                        for ( int c = 0; c < 2; c++ )
                                        {
                                            float old_var = *val[v];
                                            if ( checks[c]( val[v], prev_val[v], step[v][c] ) )
                                            {
                                                edit++;
                                                printf("[%u] Check fail (%s/%s)! Value: %f / %f\n", (unsigned int)total_count, checks_descr[c], val_descr[v], (double)old_var, (double)prev_val[v] );
                                            }
                                        }
                                    }
                                    if ( edit > 0 )
                                        edit_count++;
                                }
                                prev_temp = temp;
                                prev_ext_temp = ext_temp;
                                prev_humi = humi;
                                prev_ext_humi = ext_humi;
                                prev_epoc = epoc;
                                read_count++;

                                // Write processed set:
                                fwrite( &epoc, sizeof(epoc), 1, dst_file );
                                fwrite( &temp, sizeof(temp), 1, dst_file );
                                fwrite( &ext_temp, sizeof(ext_temp), 1, dst_file );
                                fwrite( &ext_humi, sizeof(ext_humi), 1, dst_file );
                                fwrite( &humi, sizeof(humi), 1, dst_file );
                                write_count++;

                                fprintf(plot_file, "%llu %f %f %f %f\n", (unsigned long long int)epoc, (double)temp, (double)ext_temp, (double)humi, (double)ext_humi);
//                                printf("[%u] Time: %llu Temp: %f / %f Humi: %f/%f\n", (unsigned int)total_count, (unsigned long long int)epoc, (double)temp, (double)ext_temp, (double)humi, (double)ext_humi );

                            }
                            else
                            {
                                printf("[%u] invalid! Time: %llu(%llu) Temp: %f / %f Humi: %f/%f\n", (unsigned int)total_count, (unsigned long long int)epoc,(unsigned long long int)prev_epoc, (double)temp, (double)ext_temp, (double)humi, (double)ext_humi );
                                invalid_count++;
                            }
                        }
                    }
                }
            }
        }

        total_count++;
    }

    fclose(src_file);
    fclose(dst_file);

    fclose( plot_file );

    printf("Total processed points: %u\n", total_count);
    printf("Total read points: %u\n", read_count);
    printf("Total edited points: %u\n", edit_count);
    printf("Total written points: %u\n", write_count);
    printf("Total invalid points: %u\n", invalid_count);

    return 0;
}
