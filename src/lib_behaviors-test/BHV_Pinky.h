/************************************************************/
/*    NAME: DerekOSullivan                                  */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: BHV_Pinky.h                                     */
/*    DATE: Apr 2026                                        */
/************************************************************/

#ifndef BHV_Pinky_HEADER
#define BHV_Pinky_HEADER

#include <map>
#include <set>
#include <string>
#include "IvPBehavior.h"
#include "XYPoint.h"

class BHV_Pinky : public IvPBehavior {
public:
  BHV_Pinky(IvPDomain);
  ~BHV_Pinky() {}

  bool         setParam(std::string, std::string);
  void         onIdleState();
  IvPFunction* onRunState();
  void         onEveryState(std::string);

protected:
  void         selectTarget();
  IvPFunction* buildFunction();
  void         postViewPoint(bool viewable=true);
  void         postLookaheadPoint(bool viewable=true);
  bool         updateOpponent();
  void         handleSwimmerAlert(const std::string& alert);
  void         handleFoundSwimmer(const std::string& msg);

protected: // Config
  std::string m_tmate;
  double      m_lookahead_dist;
  double      m_capture_radius;
  double      m_desired_speed;

protected: // Own state
  double m_osx, m_osy;

protected: // Opponent state
  double m_opp_x, m_opp_y, m_opp_hdg, m_opp_spd;
  bool   m_opp_known;
  double m_lookahead_x, m_lookahead_y;  // projected Pinky point

protected: // Swimmer tracking
  std::map<std::string, XYPoint> m_swimmers;
  std::set<std::string>          m_rescued_ids;

protected: // Current target
  double      m_ptx, m_pty;
  bool        m_pt_set;
  std::string m_target_id;
};

#define IVP_EXPORT_FUNCTION
extern "C" {
  IVP_EXPORT_FUNCTION IvPBehavior * createBehavior(std::string name, IvPDomain domain)
  {return new BHV_Pinky(domain);}
}
#endif
