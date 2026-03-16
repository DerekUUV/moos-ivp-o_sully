/************************************************************/
/*    NAME: DerekOSullivan                                  */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: GenPath.h                                       */
/*    DATE: 13mar26                                         */
/************************************************************/

#ifndef GenPath_HEADER
#define GenPath_HEADER

#include "MOOS/libMOOS/Thirdparty/AppCasting/AppCastingMOOSApp.h"
#include "XYPoint.h"
#include "XYSegList.h"
#include <vector>

class GenPath : public AppCastingMOOSApp
{
public:
  GenPath();
  ~GenPath();

protected: // Standard MOOSApp functions to overload
  bool OnNewMail(MOOSMSG_LIST &NewMail);
  bool Iterate();
  bool OnConnectToServer();
  bool OnStartUp();

protected: // Standard AppCastingMOOSApp function to overload
  bool buildReport();

protected:
  void registerVariables();
  XYSegList buildGreedyPath();

private: // State variables
  std::vector<XYPoint> m_vector_points; // received visit points
  bool   m_receiving;                   // currently collecting points
  bool   m_points_received;             // lastpoint has arrived
  bool   m_path_posted;                 // path waypoints have been published
  bool   m_traverse_posted;            // TRAVERSE=true has been sent (one cycle after path)
  double m_nav_x;                       // current vehicle x
  double m_nav_y;                       // current vehicle y
  std::string m_vname;                  // vehicle name (from config)
};

#endif
