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
  XYSegList buildGreedyPath(std::vector<XYPoint> points, double cx, double cy);
  XYSegList buildGreedyPath(std::vector<XYPoint> points); // starts from m_nav_x/y

private:
  void      handleNodeReport(const std::string& val);
  XYPoint   selectPinkyTarget();
  std::string selectClusterDenialTarget();
  XYSegList buildAdversarialPath(XYPoint first);

private: // State variables
  std::map<std::string, XYPoint> m_swimmers;    // id -> position (unrescued)
  std::set<std::string>          m_rescued_ids; // ids confirmed rescued

  double      m_nav_x;
  double      m_nav_y;
  std::string m_vname;
  double      m_visit_radius; // meters - within this counts as visited

  // Adversarial heuristics
  std::string m_heuristic;        // "greedy", "pinky", "secondstring", "clusterdenial"
  std::string m_tmate;            // opponent vehicle name to track
  double      m_lookahead_dist;   // meters ahead of opponent to project (pinky only)
  double      m_cluster_radius;   // neighbourhood radius for cluster scoring
  double      m_opp_x, m_opp_y, m_opp_hdg;
  bool        m_opp_known;

  // Committed first target (secondstring and clusterdenial)
  std::string m_committed_id;
  bool        m_has_committed_target;
};

#endif
