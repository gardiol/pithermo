#include "history.h"
#include "historyitem.h"

double getTemp( uint32_t h, bool interna )
{
    double ret = 0.0;
    if ( interna )
    {
        if ( h <= 7 )
            ret = 17;
        else if ( h == 8 )
            ret = 18;
        else if ( h == 9 )
            ret = 18;
        else if ( h == 10 )
            ret = 18.5;
        else if ( h == 11 )
            ret = 19;
        else if ( h == 12 )
            ret = 19.2;
        else if ( h == 13 )
            ret = 19.4;
        else if ( h == 14 )
            ret = 19.6;
        else if ( h == 15 )
            ret = 19.7;
        else if ( h == 16 )
            ret = 19.8;
        else if ( h == 17 )
            ret = 19.9;
        else if ( h == 18 )
            ret = 20;
        else if ( h == 19 )
            ret = 20.1;
        else if ( h == 20 )
            ret = 20;
        else if ( h == 21 )
            ret = 19.5;
        else if ( h == 22 )
            ret = 19;
        else
            ret = 18;
    }
    else
    {
        if ( (h <= 4) || (h >= 20) )
            ret = -1;
        else if ( h == 5 )
            ret = -0.5;
        else if ( h == 6 )
            ret = -0.3;
        else if ( h == 7 )
            ret = -0.1;
        else if ( h == 8 )
            ret = 0.0;
        else if ( h == 9 )
            ret = 0.5;
        else if ( h == 10 )
            ret = 1.5;
        else if ( h == 11)
            ret = 2.5;
        else if ( h == 12)
            ret = 4.5;
        else if ( h == 13)
            ret = 6.5;
        else if ( h == 14)
            ret = 5.5;
        else if ( h == 15)
            ret = 5.5;
        else if ( h == 16)
            ret = 4.5;
        else if ( h == 17)
            ret = 3.5;
        else if ( h == 18)
            ret = 1.5;
        else
            ret = -1;
    }
    return ret;
}

void writeItem( HistoryItem* item, FILE* dest_csv, FILE* dest_bin )
{
    struct tm ts;
    time_t now = item->getTime();
    char str_buf[1024];
    ts = *localtime(&now);
    strftime(str_buf, sizeof(str_buf), "%a %Y-%m-%d %H:%M:%S %Z", &ts);
    fprintf( dest_csv, "%llu, %s, %f, %f, %f, %f\n",
            static_cast<unsigned long long int>(now),
             str_buf,
            item->getTemp(),
            item->getHumidity(),
            item->getExtTemp(),
            item->getExtHumidity() );
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
                    HistoryItem item( source );
                    if ( item.isValid() )
                    {
                        n_records++;
                        writeItem( &item, destination, destination_bin );

                        uint64_t now = item.getTime();
                        if ( now == 1573023642 ) // ore 8:00 del 6 novembre
                        {
                            // Fill the blank 6 november
                            for ( int h = 8; h < 24; h++ )
                            {
                                now += 3600;
                                HistoryItem item2( now, getTemp(h, true), 50.0, getTemp(h,false),99.0);
                                writeItem( &item2, destination, destination_bin );
                            }
                            // Fill the blank from 7/11 to 30/11
                            for ( int d = 7; d < 31; d++ )
                            {
                                for ( int h = 0; h < 24; h++ )
                                {
                                    now += 3600;
                                    HistoryItem item2( now, getTemp(h, true), 50.0, getTemp(h,false),99.0);
                                    writeItem( &item2, destination, destination_bin );
                                }
                            }
                            // Fill the blank from 1/12 to 31/12
                            for ( int d = 1; d < 32; d++ )
                            {
                                for ( int h = 0; h < 24; h++ )
                                {
                                    now += 3600;
                                    HistoryItem item2( now, getTemp(h, true), 50.0, getTemp(h,false),99.0);
                                    writeItem( &item2, destination, destination_bin );
                                }
                            }
                            // Fill the blank from 1/1/20 to 03/01/20
                            for ( int d = 1; d < 4; d++ )
                            {
                                for ( int h = 0; h < 24; h++ )
                                {
                                    now += 3600;
                                    HistoryItem item2( now, getTemp(h, true), 50.0, getTemp(h,false),99.0);
                                    writeItem( &item2, destination, destination_bin );
                                }
                            }
                            // Fill the blank 4 jannuary
                            for ( int h = 0; h < 21; h++ )
                            {
                                now += 3600;
                                HistoryItem item2( now, getTemp(h, true), 50.0, getTemp(h,false),99.0);
                                writeItem( &item2, destination, destination_bin );
                            }
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
        printf("Usage: %s history_file\n", argv[0] );

    return ret;
}
