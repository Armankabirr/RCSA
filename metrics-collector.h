#ifndef METRICS_COLLECTOR_H
#define METRICS_COLLECTOR_H

#include <cstdint>

namespace ns3 {

/**
 * \brief Global aggregate metrics matching the paper's three evaluation
 * metrics (Section 5.1): resource searching delay, number of packets,
 * and success ratio. A simple static counter set -- reset between runs.
 */
class MetricsCollector
{
public:
  static void Reset (void);

  static void RecordRequestSent (void);
  static void RecordPacketSent (void);   // REQ/IREQ/RES packets only,
                                          // matching the paper's packet
                                          // count definition (search +
                                          // allocation traffic)
  static void RecordSuccess (double delaySeconds);
  static void RecordFailure (double delaySeconds);

  static uint32_t GetTotalRequests (void);
  static uint32_t GetTotalSuccesses (void);
  static uint32_t GetTotalFailures (void);
  static uint32_t GetTotalPackets (void);
  static double GetAverageDelay (void);       // over ALL completed requests
  static double GetAverageSuccessDelay (void); // over successful requests only
  static double GetSuccessRatio (void);        // successes / total requests

  static void PrintSummary (void);

private:
  static uint32_t s_totalRequests;
  static uint32_t s_totalSuccesses;
  static uint32_t s_totalFailures;
  static uint32_t s_totalPackets;
  static double   s_totalDelaySum;
  static double   s_successDelaySum;
};

} // namespace ns3

#endif /* METRICS_COLLECTOR_H */
