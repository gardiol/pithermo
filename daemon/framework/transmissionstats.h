#ifndef TRANSMISSIONSTATS_H
#define TRANSMISSIONSTATS_H

#include <common_defs.h>

namespace FrameworkLibrary {

class BaseCommInterface;

class __private_TransmissionStats;
/** @brief Silently manage statistics for all your BaseCommInterface objects
 *
 * This class is used to collect statistics from the BaseCommInterface objects.
 * At this point only total bytes send and received are availble in aggregated form for all interfaces.
 *
 * 
 *
 * @review hide implementation
 *
 */
class TransmissionStats
{
public:
    /** @brief Create a new statistic measure
     * @param s the interface
     *
     * 
     *
     */
    TransmissionStats( const BaseCommInterface* s);

    virtual ~TransmissionStats();

    /** @brief print status
     * @param time_s [out] interface running time, in seconds
     * @param sec_r [out] average bytes received last second
     * @param sec_s [out] average bytes sent last second
     * @param tot_r [out] total bytes received
     * @param tot_s [out] total bytes sent
     * @param wrst_r [out] worst bytes received, on average, in one second
     * @param wrst_s [out] worst bytes sent, on average, in one second
     *
     * 
     *
     */
    void getStats(uint64_t &time_s,
                  uint64_t &sec_r, uint64_t &sec_s,
                  uint64_t &tot_r, uint64_t &tot_s,
                  uint64_t &wrst_r, uint64_t &wrst_s) const;

    /** @brief add to statistics
     * @param b sent bytes
     * @return b
     *
     * 
     *
     */
    uint32_t addSentBytes( uint32_t b);

    /** @brief add to statistics
     * @param b received bytes
     * @return b
     *
     * 
     *
     */
    uint32_t addReceivedBytes( uint32_t b);

private:
    __private_TransmissionStats* _private;
};

}

#endif // TRANSMISSIONSTATS_H
