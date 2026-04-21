/************************************************************/
/*    NAME: DerekOSullivan                                  */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: GenRescue.h                                     */
/*    DATE: 13mar26                                         */
/************************************************************/

#ifndef GenRescue_HEADER
#define GenRescue_HEADER

#include "MOOS/libMOOS/Thirdparty/AppCasting/AppCastingMOOSApp.h"
#include "XYPoint.h"
#include "XYSegList.h"
#include <map>
#include <set>
#include <string>
#include <vector>

class GenRescue : public AppCastingMOOSApp
{
public:
  GenRescue();
  ~GenRescue();

protected: // Standard MOOSApp functions to overload
  bool OnNewMail(MOOSMSG_LIST &NewMail);
  bool Iterate();
  bool OnConnectToServer();
  bool OnStartUp();

protected: // Standard AppCastingMOOSApp function to overload
  bool buildReport();

protected:
  void registerVariables();
  void handleSwimmerAlert(const std::string& val);
  void handleFoundSwimmer(const std::string& val);
  void postUpdatedPath();
  XYSegList buildGreedyPath(std::vector<XYPoint> points);

private: // State variables
  std::map<std::string, XYPoint> m_swimmers;    // id -> position (unrescued)
  std::set<std::string>          m_rescued_ids; // ids confirmed rescued

  double      m_nav_x;
  double      m_nav_y;
  std::string m_vname;
  double      m_visit_radius; // meters - within this counts as visited
};

#endif
