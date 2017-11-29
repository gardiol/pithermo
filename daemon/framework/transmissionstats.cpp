#include "transmissionstats.h"
#include "basecomminterface.h"

using namespace FrameworkLibrary;

namespace FrameworkLibrary {

class __private_TransmissionStats
{
public:
    explicit __private_TransmissionStats( const BaseCommInterface* s ):
        _timer(true),
        _mutex("TransmissionStats")
    {
        _parent = s;
        _sent_bytes = 0;
        _received_bytes = 0;

        _last_second_s = 0;
        _cur_second_recv_b = 0;
        _cur_second_sent_b = 0;
        _last_second_recv_b = 0;
        _last_second_sent_b = 0;
        _worst_case_recv_b = 0;
        _worst_case_sent_b = 0;
    }

    void addSent( uint64_t b )
    {
        _sent_bytes += b;
        _testSecond();
        _cur_second_sent_b += b;
    }

    void addRecv( uint64_t b )
    {
        _received_bytes += b;
        _testSecond();
        _cur_second_recv_b += b;
    }

    void getStats(uint64_t& t,
                  uint64_t &sec_r, uint64_t &sec_s,
                  uint64_t &r, uint64_t &s ,
                  uint64_t &wrst_r, uint64_t &wrst_s) const
    {
        t =  _last_second_s;
        sec_r = _last_second_recv_b;
        sec_s = _last_second_sent_b;
        r = _received_bytes;
        s = _sent_bytes;
        wrst_r = _worst_case_recv_b;
        wrst_s = _worst_case_sent_b;
    }

private:
    void _testSecond()
    {
        _mutex.lock();
        uint64_t new_second = _timer.elapsedTimeS();
        if ( _last_second_s != new_second )
        {
            _last_second_recv_b = _cur_second_recv_b;
            _last_second_sent_b = _cur_second_sent_b;
            if ( _last_second_recv_b > _worst_case_recv_b )
                _worst_case_recv_b = _last_second_recv_b;
            if ( _last_second_sent_b > _worst_case_sent_b )
                _worst_case_sent_b = _last_second_sent_b;
            _last_second_s = new_second;
            _cur_second_recv_b = 0;
            _cur_second_sent_b = 0;
        }
        _mutex.unlock();
    }

    const BaseCommInterface* _parent;
    FrameworkTimer _timer;
    BaseMutex _mutex;
    uint64_t _sent_bytes;
    uint64_t _received_bytes;

    uint64_t _last_second_s;
    uint64_t _cur_second_recv_b;
    uint64_t _cur_second_sent_b;
    uint64_t _last_second_recv_b;
    uint64_t _last_second_sent_b;
    uint64_t _worst_case_recv_b;
    uint64_t _worst_case_sent_b;

};

}

TransmissionStats::TransmissionStats(const BaseCommInterface *s)
{
    _private = new __private_TransmissionStats( s );
}

TransmissionStats::~TransmissionStats()
{
    delete _private;
    _private = NULL;
}

void TransmissionStats::getStats(uint64_t& t,
                                 uint64_t &sec_r, uint64_t &sec_s,
                                 uint64_t &r, uint64_t &s ,
                                 uint64_t &wrst_r, uint64_t &wrst_s) const
{
    _private->getStats( t, sec_r, sec_s, r, s, wrst_r, wrst_s );
}

uint32_t TransmissionStats::addSentBytes(uint32_t b)
{
    if ( b > 0 )
        _private->addSent( b );
    return b;
}

uint32_t TransmissionStats::addReceivedBytes(uint32_t b)
{
    if ( b > 0 )
        _private->addRecv( b );
    return b;
}
