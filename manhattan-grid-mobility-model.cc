#include "manhattan-grid-mobility-model.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include <algorithm>
#include <cmath>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ManhattanGridMobilityModel");
NS_OBJECT_ENSURE_REGISTERED (ManhattanGridMobilityModel);

TypeId
ManhattanGridMobilityModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ManhattanGridMobilityModel")
    .SetParent<MobilityModel> ()
    .SetGroupName ("RCSA_SIM")
    .AddConstructor<ManhattanGridMobilityModel> ();
  return tid;
}

ManhattanGridMobilityModel::ManhattanGridMobilityModel ()
{
  m_position = Vector (0.0, 0.0, 0.0);
  m_velocity = Vector (0.0, 0.0, 0.0);
  m_timestep = Seconds (1.0);

  m_maxAccel  = 5.0;
  m_minAccel  = -5.0;
  m_maxSpeed  = 40.0 / 3.6;
  m_minSpeed  = 0.0;
  m_variance  = 1.0;

  m_gridSize  = 2000.0;
  m_blockSize = 1000.0;
  m_speed     = 0.0;
  m_direction = 0;

  m_probStraight = 0.7;
  m_probTurn     = 0.1;
  m_probBack     = 0.1;

  m_accelNoise = CreateObject<NormalRandomVariable> ();
  m_dirRandom  = CreateObject<UniformRandomVariable> ();
}

ManhattanGridMobilityModel::~ManhattanGridMobilityModel ()
{
}

void
ManhattanGridMobilityModel::DoInitialize (void)
{
  SnapToNearestIntersection ();
  ChooseDirectionAtIntersection ();
  UpdatePosition ();
  MobilityModel::DoInitialize ();
}

Vector
ManhattanGridMobilityModel::DoGetPosition (void) const
{
  return m_position;
}

void
ManhattanGridMobilityModel::DoSetPosition (const Vector &position)
{
  m_position = position;
  NotifyCourseChange ();
}

Vector
ManhattanGridMobilityModel::DoGetVelocity (void) const
{
  return m_velocity;
}

Ptr<MobilityModel>
ManhattanGridMobilityModel::Copy (void) const
{
  Ptr<ManhattanGridMobilityModel> copy = CreateObject<ManhattanGridMobilityModel> ();
  copy->m_position = m_position;
  copy->m_velocity = m_velocity;
  copy->m_timestep = m_timestep;
  return copy;
}

void
ManhattanGridMobilityModel::SnapToNearestIntersection (void)
{
  double x = std::round (m_position.x / m_blockSize) * m_blockSize;
  double y = std::round (m_position.y / m_blockSize) * m_blockSize;
  x = std::max (0.0, std::min (m_gridSize, x));
  y = std::max (0.0, std::min (m_gridSize, y));
  m_position = Vector (x, y, 0.0);
}

int
ManhattanGridMobilityModel::OppositeDirection (int direction) const
{
  return direction ^ 1;
}

double
ManhattanGridMobilityModel::NextGridLine (double coord, double delta) const
{
  double ratio = coord / m_blockSize;
  if (delta > 0.0)
    {
      double nextIndex = std::floor (ratio + 1e-9) + 1.0;
      return nextIndex * m_blockSize;
    }
  else
    {
      double prevIndex = std::ceil (ratio - 1e-9) - 1.0;
      return prevIndex * m_blockSize;
    }
}

void
ManhattanGridMobilityModel::ChooseDirectionAtIntersection (void)
{
  bool canPlusX  = (m_position.x + m_blockSize) <= m_gridSize + 1e-6;
  bool canMinusX = (m_position.x - m_blockSize) >= -1e-6;
  bool canPlusY  = (m_position.y + m_blockSize) <= m_gridSize + 1e-6;
  bool canMinusY = (m_position.y - m_blockSize) >= -1e-6;

  bool valid[4] = { canPlusX, canMinusX, canPlusY, canMinusY };

  double weight[4] = { 0.0, 0.0, 0.0, 0.0 };
  int straightDir = m_direction;
  int backDir     = OppositeDirection (m_direction);

  for (int i = 0; i < 4; ++i)
    {
      if (i == straightDir) { weight[i] = m_probStraight; }
      else if (i == backDir) { weight[i] = m_probBack; }
      else { weight[i] = m_probTurn; }
    }

  double total = 0.0;
  for (int i = 0; i < 4; ++i)
    {
      if (!valid[i]) { weight[i] = 0.0; }
      total += weight[i];
    }

  if (total <= 0.0)
    {
      m_direction = 0;
      return;
    }

  double r = m_dirRandom->GetValue (0.0, total);
  double cumulative = 0.0;
  int chosen = straightDir;
  for (int i = 0; i < 4; ++i)
    {
      cumulative += weight[i];
      if (r <= cumulative)
        {
          chosen = i;
          break;
        }
    }

  m_direction = chosen;
}

void
ManhattanGridMobilityModel::SetMaxSpeedKmh (double kmh)
{
  double target = kmh / 3.6; // target speed in m/s
  m_maxSpeed = target * 1.1;  // narrow ceiling just above target
  m_minSpeed = target * 0.7;  // narrow floor just below target, so noise
                               // fluctuates AROUND the target with less
                               // spread than before (less cluster churn)
  m_speed = target * 0.85;    // start slightly below target, closer to
                               // original baseline dynamics
}

void
ManhattanGridMobilityModel::UpdatePosition (void)
{
  double dt = m_timestep.GetSeconds ();

  double accel = m_accelNoise->GetValue (0.0, m_variance);
  accel = std::max (m_minAccel, std::min (m_maxAccel, accel));

  m_speed += accel * dt;
  m_speed = std::max (m_minSpeed, std::min (m_maxSpeed, m_speed));

  double dx = 0.0, dy = 0.0;
  switch (m_direction)
    {
    case 0: dx = m_speed * dt; break;
    case 1: dx = -m_speed * dt; break;
    case 2: dy = m_speed * dt; break;
    case 3: dy = -m_speed * dt; break;
    }

  Vector oldPos = m_position;
  bool movingX = (dx != 0.0);
  double axisCoord = movingX ? oldPos.x : oldPos.y;
  double axisDelta = movingX ? dx : dy;

  double newAxisCoord = axisCoord + axisDelta;
  bool crossed = false;

  if (axisDelta != 0.0)
    {
      double nextLine = NextGridLine (axisCoord, axisDelta);
      if (axisDelta > 0.0 && newAxisCoord >= nextLine)
        {
          newAxisCoord = nextLine;
          crossed = true;
        }
      else if (axisDelta < 0.0 && newAxisCoord <= nextLine)
        {
          newAxisCoord = nextLine;
          crossed = true;
        }
    }

  Vector newPos = oldPos;
  if (movingX) { newPos.x = newAxisCoord; }
  else { newPos.y = newAxisCoord; }

  newPos.x = std::max (0.0, std::min (m_gridSize, newPos.x));
  newPos.y = std::max (0.0, std::min (m_gridSize, newPos.y));

  m_position = newPos;

  double actualDx = newPos.x - oldPos.x;
  double actualDy = newPos.y - oldPos.y;
  m_velocity = Vector (actualDx / dt, actualDy / dt, 0.0);

  if (crossed)
    {
      SnapToNearestIntersection ();
      ChooseDirectionAtIntersection ();
    }

  DoSetPosition (m_position);

  m_updateEvent = Simulator::Schedule (m_timestep,
                                        &ManhattanGridMobilityModel::UpdatePosition,
                                        this);
}

} // namespace ns3
