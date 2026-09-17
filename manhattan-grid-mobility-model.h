#ifndef MANHATTAN_GRID_MOBILITY_MODEL_H
#define MANHATTAN_GRID_MOBILITY_MODEL_H

#include "ns3/mobility-model.h"
#include "ns3/vector.h"
#include "ns3/event-id.h"
#include "ns3/random-variable-stream.h"

namespace ns3 {

class ManhattanGridMobilityModel : public MobilityModel
{
public:
  static TypeId GetTypeId (void);

  ManhattanGridMobilityModel ();
  virtual ~ManhattanGridMobilityModel ();

  void SetMaxSpeedKmh (double kmh);

private:
  virtual Vector DoGetPosition (void) const;
  virtual void DoSetPosition (const Vector &position);
  virtual Vector DoGetVelocity (void) const;
  virtual Ptr<MobilityModel> Copy (void) const;
  virtual void DoInitialize (void);

  void UpdatePosition (void);
  void ChooseDirectionAtIntersection (void);
  void SnapToNearestIntersection (void);
  int  OppositeDirection (int direction) const;
  double NextGridLine (double coord, double delta) const;

  Vector m_position;
  Vector m_velocity;
  Time   m_timestep;
  EventId m_updateEvent;

  Ptr<NormalRandomVariable> m_accelNoise;
  Ptr<UniformRandomVariable> m_dirRandom;

  double m_maxAccel;
  double m_minAccel;
  double m_maxSpeed;
  double m_minSpeed;
  double m_variance;

  double m_gridSize;
  double m_blockSize;
  double m_speed;
  int    m_direction;

  double m_probStraight;
  double m_probTurn;
  double m_probBack;
};

} // namespace ns3

#endif /* MANHATTAN_GRID_MOBILITY_MODEL_H */
