#include "history.h"
#include "historyitem.h"


int main( int argc, char** argv )
{
    int ret = 255;
    if ( argc == 2 )
    {
        std::string src = argv[1];
        std::string dst = src + ".csv";

        printf("Exporting %s to %s.csv...\n", src.c_str(), dst.c_str() );

        FILE* source = fopen( src.c_str(), "rb" );
        if ( source != nullptr )
        {
            FILE* destination = fopen( dst.c_str(), "w" );
            if ( destination != nullptr )
            {
                uint32_t n_records = 0;
                char str_buf[1024];
                while ( !feof( source ) )
                {
                    HistoryItem item( source );
                    if ( item.isValid() )
                    {
                        n_records++;
                        time_t now = item.getTime();
                        struct tm ts = *localtime(&now);
                        strftime(str_buf, sizeof(str_buf), "%a %Y-%m-%d %H:%M:%S %Z", &ts);
                        fprintf( destination, "%llu, %s, %f, %f, %f, %f\n",
                                static_cast<unsigned long long int>(item.getTime()),
                                 str_buf,
                                item.getTemp(),
                                item.getHumidity(),
                                item.getExtTemp(),
                                item.getExtHumidity() );
                    }
                }
                printf("Exported %d records.\n", n_records );
                fclose( destination );
            }
            else
                printf("Error: unable to open destination file %s\n", dst.c_str() );
            fclose( source );
        }
        else
            printf("Error: unable to open source file %s\n", src.c_str() );
    }
    else
        printf("Usage: %s history_file\n", argv[0] );

    return ret;
}
