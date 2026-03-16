/************************************************************/
/*    NAME: DerekOSullivan                                  */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: PointAssign.h                                   */
/*    DATE: 10mar26                                         */
/************************************************************/

#ifndef PointAssign_HEADER
#define PointAssign_HEADER

#include "MOOS/libMOOS/Thirdparty/AppCasting/AppCastingMOOSApp.h"
#include "XYPoint.h"
#include <vector>
#include <string>

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

private: // State variables
  bool   m_assign_by_region;
  int    m_next_index;
  double m_mid_x;
  bool   m_mid_x_computed;
  bool   m_timer_started;
  bool   m_henry_ready;
  bool   m_gilda_ready;

  std::vector<std::string> m_vnames;
  std::vector<std::string> m_visit_points;
};

#endif
