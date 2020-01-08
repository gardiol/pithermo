#include "logger.h"
#include "logitem.h"

void writeItem( LogItem* item, FILE* dest_csv, FILE* dest_bin )
{
    struct tm ts;
    time_t now = item->getTime();
    char str_buf[1024];
    ts = *localtime(&now);
    strftime(str_buf, sizeof(str_buf), "%a %Y-%m-%d %H:%M:%S %Z", &ts);
    fprintf( dest_csv, "%llu, %s, %llu\n",
            static_cast<unsigned long long int>(now),
             str_buf,
             static_cast<unsigned long long int>( item->getEvent() ) );
    item->write( dest_bin );
}


int main( int argc, char** argv )
{
    int ret = 255;
    if ( argc == 2 )
    {
        std::string src = argv[1];
        std::string dst = src + ".csv";
        std::string dst_bin = src + ".mod";

        printf("Exporting %s to %s and %s...\n", src.c_str(), dst.c_str(), dst_bin.c_str() );

        FILE* source = fopen( src.c_str(), "rb" );
        if ( source != nullptr )
        {
            FILE* destination = fopen( dst.c_str(), "w" );
            FILE* destination_bin = fopen( dst_bin.c_str(), "wb" );
            if ( (destination != nullptr) && (destination_bin != nullptr) )
            {
                uint32_t n_records = 0;
                while ( !feof( source ) )
                {
                    LogItem item( source );
                    if ( item.isValid() )
                    {
                        n_records++;
                        writeItem( &item, destination, destination_bin );

                        uint64_t now = item.getTime();
                        if ( now == 1573022824 ) // ore 8:00 del 6 novembre
                        {
                            LogItem ev1( LogItem::GAS_OFF,now+1 );
                            writeItem( &ev1, destination, destination_bin );
                            LogItem ev2( LogItem::PELLET_OFF,now+1);
                            writeItem( &ev2, destination, destination_bin );
                            LogItem ev3( LogItem::PELLET_MODULATION,now+1 );
                            writeItem( &ev3, destination, destination_bin );
                        }
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
        printf("Usage: %s events_file\n", argv[0] );

    return ret;
}
