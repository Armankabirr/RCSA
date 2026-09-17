#include "cluster-scoring.h"

namespace ns3 {

double
ClusterScoring::GetConnectionTimeWeight (void)
{
  return 1.0; // 1 point per second of connection duration; tune later
}

double
ClusterScoring::ComputeScore (double resourceAmount, Time connectionDuration)
{
  return resourceAmount + GetConnectionTimeWeight () * connectionDuration.GetSeconds ();
}

bool
ClusterScoring::AWins (uint32_t idA, double scoreA,
                        uint32_t idB, double scoreB)
{
  if (scoreA != scoreB)
    {
      return scoreA > scoreB;
    }
  return idA < idB; // deterministic tie-break
}

} // namespace ns3
