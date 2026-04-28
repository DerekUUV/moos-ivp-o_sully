/************************************************************/
/*    NAME: DerekOSullivan                                  */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: BHV_Pinky.cpp                                   */
/*    DATE: Apr 2026                                        */
/************************************************************/

#include <cmath>
#include "BHV_Pinky.h"
#include "MBUtils.h"
#include "AngleUtils.h"
#include "BuildUtils.h"
#include "ZAIC_PEAK.h"
#include "OF_Coupler.h"

using namespace std;

//-----------------------------------------------------------
// Constructor

BHV_Pinky::BHV_Pinky(IvPDomain gdomain) : IvPBehavior(gdomain)
{
  IvPBehavior::setParam("name", "pinky");
  m_domain = subDomain(m_domain, "course,speed");

  m_tmate          = "";
  m_lookahead_dist = 20.0;
  m_capture_radius = 5.0;
  m_desired_speed  = 1.2;

  m_osx = 0; m_osy = 0;

  m_opp_x = 0; m_opp_y = 0; m_opp_hdg = 0; m_opp_spd = 0;
  m_opp_known = false;
  m_lookahead_x = 0; m_lookahead_y = 0;

  m_ptx = 0; m_pty = 0;
  m_pt_set = false;

  addInfoVars("NAV_X, NAV_Y");
  addInfoVars("NODE_REPORT");
  addInfoVars("SWIMMER_ALERT");
  addInfoVars("FOUND_SWIMMER");
}

//---------------------------------------------------------------
// Procedure: setParam

bool BHV_Pinky::setParam(string param, string val)
{
  param = tolower(param);

  if(param == "tmate")
    return setNonWhiteVarOnString(m_tmate, val);
  else if(param == "lookahead_dist")
    return setPosDoubleOnString(m_lookahead_dist, val);
  else if(param == "capture_radius")
    return setPosDoubleOnString(m_capture_radius, val);
  else if(param == "desired_speed")
    return setPosDoubleOnString(m_desired_speed, val);

  return false;
}

//-----------------------------------------------------------
// Procedure: onEveryState
//   Keeps swimmer map and opponent state current regardless of
//   whether the behavior is active or idle.

void BHV_Pinky::onEveryState(string)
{
  if(getBufferVarUpdated("SWIMMER_ALERT"))
    handleSwimmerAlert(getBufferStringVal("SWIMMER_ALERT"));
  if(getBufferVarUpdated("FOUND_SWIMMER"))
    handleFoundSwimmer(getBufferStringVal("FOUND_SWIMMER"));
  if(getBufferVarUpdated("NODE_REPORT"))
    updateOpponent();
}

//-----------------------------------------------------------
// Procedure: onIdleState

void BHV_Pinky::onIdleState()
{
  postViewPoint(false);
  postLookaheadPoint(false);
}

//-----------------------------------------------------------
// Procedure: onRunState

IvPFunction* BHV_Pinky::onRunState()
{
  bool ok1, ok2;
  m_osx = getBufferDoubleVal("NAV_X", ok1);
  m_osy = getBufferDoubleVal("NAV_Y", ok2);
  if(!ok1 || !ok2) {
    postWMessage("No ownship X/Y info in info_buffer.");
    return 0;
  }

  // Check if we reached the current target
  if(m_pt_set) {
    double dist = hypot(m_ptx - m_osx, m_pty - m_osy);
    if(dist <= m_capture_radius) {
      // Locally mark rescued; FOUND_SWIMMER from shore will confirm
      if(!m_target_id.empty()) {
        m_rescued_ids.insert(m_target_id);
        m_swimmers.erase(m_target_id);
      }
      m_pt_set = false;
      postViewPoint(false);
    }
  }

  if(!m_pt_set)
    selectTarget();

  if(!m_pt_set) {
    postWMessage("No swimmer targets known yet");
    return 0;
  }

  postViewPoint(true);
  postLookaheadPoint(m_opp_known);
  return buildFunction();
}

//-----------------------------------------------------------
// Procedure: selectTarget
//   Pinky strategy: project the opponent forward, find the
//   swimmer nearest that lookahead point, race there if we
//   can arrive before the opponent. Otherwise go greedy.

void BHV_Pinky::selectTarget()
{
  if(m_swimmers.empty())
    return;

  // Update lookahead point
  if(m_opp_known) {
    double hdg_rad = m_opp_hdg * M_PI / 180.0;
    m_lookahead_x = m_opp_x + m_lookahead_dist * sin(hdg_rad);
    m_lookahead_y = m_opp_y + m_lookahead_dist * cos(hdg_rad);
  } else {
    // No opponent info yet — use own position as fallback lookahead
    m_lookahead_x = m_osx;
    m_lookahead_y = m_osy;
  }

  string pinky_id = "";
  double pinky_la_dist = 1e9;   // swimmer dist to lookahead point

  string greedy_id = "";
  double greedy_my_dist = 1e9;  // swimmer dist to us

  for(map<string, XYPoint>::iterator it = m_swimmers.begin();
      it != m_swimmers.end(); ++it) {
    double sx = it->second.x(), sy = it->second.y();

    double la_dist = hypot(m_lookahead_x - sx, m_lookahead_y - sy);
    if(la_dist < pinky_la_dist) {
      pinky_la_dist = la_dist;
      pinky_id      = it->first;
    }

    double my_dist = hypot(m_osx - sx, m_osy - sy);
    if(my_dist < greedy_my_dist) {
      greedy_my_dist = my_dist;
      greedy_id      = it->first;
    }
  }

  // Use Pinky target only if we can beat the opponent there
  string chosen_id = greedy_id;
  if(!pinky_id.empty() && m_opp_known) {
    double sx = m_swimmers[pinky_id].x(), sy = m_swimmers[pinky_id].y();
    double my_dist  = hypot(m_osx - sx, m_osy - sy);
    double opp_dist = hypot(m_opp_x - sx, m_opp_y - sy);
    if(my_dist < opp_dist)
      chosen_id = pinky_id;
  }

  if(chosen_id.empty())
    return;

  m_target_id = chosen_id;
  m_ptx       = m_swimmers[chosen_id].x();
  m_pty       = m_swimmers[chosen_id].y();
  m_pt_set    = true;
}

//-----------------------------------------------------------
// Procedure: buildFunction

IvPFunction* BHV_Pinky::buildFunction()
{
  if(!m_pt_set)
    return 0;

  ZAIC_PEAK spd_zaic(m_domain, "speed");
  spd_zaic.setSummit(m_desired_speed);
  spd_zaic.setPeakWidth(0.5);
  spd_zaic.setBaseWidth(1.0);
  spd_zaic.setSummitDelta(0.8);
  if(!spd_zaic.stateOK()) {
    postWMessage("Speed ZAIC problems: " + spd_zaic.getWarnings());
    return 0;
  }

  double rel_ang = relAng(m_osx, m_osy, m_ptx, m_pty);
  ZAIC_PEAK crs_zaic(m_domain, "course");
  crs_zaic.setSummit(rel_ang);
  crs_zaic.setPeakWidth(0);
  crs_zaic.setBaseWidth(180.0);
  crs_zaic.setSummitDelta(0);
  crs_zaic.setValueWrap(true);
  if(!crs_zaic.stateOK()) {
    postWMessage("Course ZAIC problems: " + crs_zaic.getWarnings());
    return 0;
  }

  IvPFunction* spd_ipf = spd_zaic.extractIvPFunction();
  IvPFunction* crs_ipf = crs_zaic.extractIvPFunction();

  OF_Coupler coupler;
  IvPFunction* ivp = coupler.couple(crs_ipf, spd_ipf, 50, 50);
  if(ivp)
    ivp->setPWT(m_priority_wt);
  return ivp;
}

//-----------------------------------------------------------
// Procedure: postViewPoint

void BHV_Pinky::postViewPoint(bool viewable)
{
  XYPoint pt(m_ptx, m_pty);
  pt.set_vertex_size(6);
  pt.set_vertex_color("magenta");
  pt.set_label(m_us_name + "_pinky_target");
  string spec = viewable ? pt.get_spec("active=true") : pt.get_spec("active=false");
  postMessage("VIEW_POINT", spec);
}

//-----------------------------------------------------------
// Procedure: postLookaheadPoint
//   Visualizes the projected Pinky interception point.

void BHV_Pinky::postLookaheadPoint(bool viewable)
{
  XYPoint pt(m_lookahead_x, m_lookahead_y);
  pt.set_vertex_size(4);
  pt.set_vertex_color("pink");
  pt.set_label(m_us_name + "_pinky_lookahead");
  string spec = viewable ? pt.get_spec("active=true") : pt.get_spec("active=false");
  postMessage("VIEW_POINT", spec);
}

//-----------------------------------------------------------
// Procedure: updateOpponent
//   Parses NODE_REPORT and updates opponent state if the
//   report is from our designated tmate (opponent vehicle).

bool BHV_Pinky::updateOpponent()
{
  if(m_tmate.empty()) {
    postWMessage("No opponent vehicle name (tmate) set");
    return false;
  }

  string report = getBufferStringVal("NODE_REPORT");
  string name   = tokStringParse(report, "NAME", ',', '=');
  if(name != m_tmate)
    return false;

  string xstr   = tokStringParse(report, "X",   ',', '=');
  string ystr   = tokStringParse(report, "Y",   ',', '=');
  string hdgstr = tokStringParse(report, "HDG", ',', '=');
  string spdstr = tokStringParse(report, "SPD", ',', '=');

  if(xstr.empty() || ystr.empty() || hdgstr.empty())
    return false;

  m_opp_x   = atof(xstr.c_str());
  m_opp_y   = atof(ystr.c_str());
  m_opp_hdg = atof(hdgstr.c_str());
  m_opp_spd = spdstr.empty() ? 0 : atof(spdstr.c_str());
  m_opp_known = true;
  return true;
}

//-----------------------------------------------------------
// Procedure: handleSwimmerAlert

void BHV_Pinky::handleSwimmerAlert(const string& alert)
{
  string xstr = tokStringParse(alert, "x",  ',', '=');
  string ystr = tokStringParse(alert, "y",  ',', '=');
  string id   = tokStringParse(alert, "id", ',', '=');

  if(xstr.empty() || ystr.empty() || id.empty()) {
    postWMessage("Malformed SWIMMER_ALERT: " + alert);
    return;
  }

  if(m_rescued_ids.count(id))
    return;

  XYPoint pt(atof(xstr.c_str()), atof(ystr.c_str()));
  pt.set_label(id);
  m_swimmers[id] = pt;
}

//-----------------------------------------------------------
// Procedure: handleFoundSwimmer

void BHV_Pinky::handleFoundSwimmer(const string& msg)
{
  string id = tokStringParse(msg, "id", ',', '=');
  if(id.empty())
    return;

  m_rescued_ids.insert(id);
  m_swimmers.erase(id);

  // Force re-selection if the opponent snagged our target
  if(id == m_target_id) {
    m_pt_set = false;
    postViewPoint(false);
  }
}
