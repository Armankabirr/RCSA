#ifndef CLUSTER_SCORING_H
#define CLUSTER_SCORING_H

#include "ns3/nstime.h"

namespace ns3 {

/**
 * \brief Scoring/comparison logic for RCH election, approximating the
 * paper's "longest connection time + largest resource amount" criterion
 * (Section 4.1.2) with a simple weighted score, since the paper's full
 * connection-probability model (Eq. 1-15) is not implemented live here.
 */
class ClusterScoring
{
public:
  // Weight balancing resource amount vs. connection duration in the score.
  // Tunable later against the paper's reported results.
  static double GetConnectionTimeWeight (void);

  // Computes a candidate's RCH-worthiness score.
  static double ComputeScore (double resourceAmount, Time connectionDuration);

  // Returns true if candidate A should win (become/remain CH) over candidate B.
  // Ties are broken by lower node ID for determinism.
  static bool AWins (uint32_t idA, double scoreA,
                      uint32_t idB, double scoreB);
};

} // namespace ns3

#endif /* CLUSTER_SCORING_H */
