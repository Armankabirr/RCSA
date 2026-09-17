#ifndef RESOURCE_INFO_H
#define RESOURCE_INFO_H

#include "ns3/object.h"

namespace ns3 {

/**
 * \brief Holds a vehicle's resource type and available resource amount.
 * Matches the paper's concept of vehicles carrying a single resource
 * type (e.g. storage, bandwidth) used for Resource Cluster construction.
 */
class ResourceInfo : public Object
{
public:
  static TypeId GetTypeId (void);

  ResourceInfo ();
  virtual ~ResourceInfo ();

  void SetResourceType (uint32_t type);
  uint32_t GetResourceType (void) const;

  void SetResourceAmount (double amount);
  double GetResourceAmount (void) const;

  void SetAvailableAmount (double amount);
  double GetAvailableAmount (void) const;

private:
  uint32_t m_resourceType;
  double   m_resourceAmount;   // total resource capacity
  double   m_availableAmount;  // currently unallocated portion
};

} // namespace ns3

#endif /* RESOURCE_INFO_H */
