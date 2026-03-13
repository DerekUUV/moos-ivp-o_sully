/************************************************************/
/*    NAME: DerekOSullivan                                              */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: PointAssign.h                                          */
/*    DATE: December 29th, 1963                             */
/************************************************************/

#ifndef PointAssign_HEADER
#define PointAssign_HEADER

#include "MOOS/libMOOS/Thirdparty/AppCasting/AppCastingMOOSApp.h"
#include "XYPoint.h"
#include <map>

class PointAssign : public AppCastingMOOSApp
{
public:
  PointAssign();
  ~PointAssign();

protected: // Standard MOOSApp functions to overload
  bool OnNewMail(MOOSMSG_LIST &NewMail);
  bool Iterate();
  bool OnConnectToServer();
  bool OnStartUp();

protected: // Standard AppCastingMOOSApp function to overload
  bool buildReport();

protected:
  void registerVariables();
  void postViewPoint(double x, double y, std::string label, std::string color);

private: // Configuration variables
  std::vector<XYPoint> m_vector_points;
  std::vector<std::string> m_vector_vehicles;

private: // State variables
  bool   assign_by_region;
  bool   m_points_assigned;
  double m_median_x;
  std::map<std::string, int> m_map_vehicle_count;
};

#endif
