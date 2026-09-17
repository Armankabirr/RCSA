#include "metrics-collector.h"
#include <iostream>

namespace ns3 {

uint32_t MetricsCollector::s_totalRequests = 0;
uint32_t MetricsCollector::s_totalSuccesses = 0;
uint32_t MetricsCollector::s_totalFailures = 0;
uint32_t MetricsCollector::s_totalPackets = 0;
double   MetricsCollector::s_totalDelaySum = 0.0;
double   MetricsCollector::s_successDelaySum = 0.0;

void
MetricsCollector::Reset (void)
{
  s_totalRequests = 0;
  s_totalSuccesses = 0;
  s_totalFailures = 0;
  s_totalPackets = 0;
  s_totalDelaySum = 0.0;
  s_successDelaySum = 0.0;
}

void
MetricsCollector::RecordRequestSent (void)
{
  s_totalRequests++;
}

void
MetricsCollector::RecordPacketSent (void)
{
  s_totalPackets++;
}

void
MetricsCollector::RecordSuccess (double delaySeconds)
{
  s_totalSuccesses++;
  s_totalDelaySum += delaySeconds;
  s_successDelaySum += delaySeconds;
}

void
MetricsCollector::RecordFailure (double delaySeconds)
{
  s_totalFailures++;
  s_totalDelaySum += delaySeconds;
}

uint32_t
MetricsCollector::GetTotalRequests (void) { return s_totalRequests; }

uint32_t
MetricsCollector::GetTotalSuccesses (void) { return s_totalSuccesses; }

uint32_t
MetricsCollector::GetTotalFailures (void) { return s_totalFailures; }

uint32_t
MetricsCollector::GetTotalPackets (void) { return s_totalPackets; }

double
MetricsCollector::GetAverageDelay (void)
{
  uint32_t completed = s_totalSuccesses + s_totalFailures;
  return completed > 0 ? s_totalDelaySum / completed : 0.0;
}

double
MetricsCollector::GetAverageSuccessDelay (void)
{
  return s_totalSuccesses > 0 ? s_successDelaySum / s_totalSuccesses : 0.0;
}

double
MetricsCollector::GetSuccessRatio (void)
{
  return s_totalRequests > 0
    ? static_cast<double> (s_totalSuccesses) / s_totalRequests
    : 0.0;
}

void
MetricsCollector::PrintSummary (void)
{
  std::cout << "\n=== Metrics Summary ===" << std::endl;
  std::cout << "Total requests:        " << s_totalRequests << std::endl;
  std::cout << "Successes:             " << s_totalSuccesses << std::endl;
  std::cout << "Failures:              " << s_totalFailures << std::endl;
  std::cout << "Success ratio:         " << GetSuccessRatio () << std::endl;
  std::cout << "Total packets:         " << s_totalPackets << std::endl;
  std::cout << "Avg delay (all):       " << GetAverageDelay () << "s" << std::endl;
  std::cout << "Avg delay (success):   " << GetAverageSuccessDelay () << "s" << std::endl;
}

} // namespace ns3
