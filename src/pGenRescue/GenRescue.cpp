/************************************************************/
/*    NAME: DerekOSullivan                                  */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: GenRescue.cpp                                   */
/*    DATE: 13mar26                                         */
/************************************************************/

#include <cmath> 
#include "MBUtils.h"
#include "ACTable.h"
#include "GenRescue.h"
#include "XYFormatUtilsPoint.h"

using namespace std;

//---------------------------------------------------------
// Constructor()

GenRescue::GenRescue()
{
  m_nav_x        = 0;
  m_nav_y        = 0;
  m_vname        = "";
  m_visit_radius = 5;

  m_heuristic      = "greedy";
  m_tmate          = "";
  m_lookahead_dist = 20.0;
  m_cluster_radius = 30.0;
  m_opp_x = 0; m_opp_y = 0; m_opp_hdg = 0;
  m_opp_known = false;

  m_committed_id         = "";
  m_has_committed_target = false;
}

//---------------------------------------------------------
// Destructor

GenRescue::~GenRescue()
{
}

//---------------------------------------------------------
// Procedure: OnNewMail()
//   Called whenever new MOOS messages arrive. Routes each
//   message to the appropriate handler or state variable.

bool GenRescue::OnNewMail(MOOSMSG_LIST &NewMail)
{
  AppCastingMOOSApp::OnNewMail(NewMail);

  MOOSMSG_LIST::iterator p;
  for(p = NewMail.begin(); p != NewMail.end(); p++) {
    CMOOSMsg &msg = *p;
    string key    = msg.GetKey();

    if(key == "NAV_X")
      m_nav_x = msg.GetDouble();
    else if(key == "NAV_Y")
      m_nav_y = msg.GetDouble();
    else if(key == "SWIMMER_ALERT")
      handleSwimmerAlert(msg.GetString());
    else if(key == "FOUND_SWIMMER")
      handleFoundSwimmer(msg.GetString());
    else if(key == "NODE_REPORT")
      handleNodeReport(msg.GetString());
    else if(key != "APPCAST_REQ")
      reportRunWarning("Unhandled Mail: " + key);
  }
  return(true);
}

//---------------------------------------------------------
// Procedure: handleSwimmerAlert()
//   Parses: x=34, y=85, id=21
//   Adds the swimmer to the active map if not yet rescued.
//   Re-posts the waypoint path after any change.

void GenRescue::handleSwimmerAlert(const string& val)
{
  // Pull the x, y, and id fields out of the comma-separated string
  string xstr = tokStringParse(val, "x",  ',', '=');
  string ystr = tokStringParse(val, "y",  ',', '=');
  string id   = tokStringParse(val, "id", ',', '=');

  // If any field is missing the message is malformed — warn and skip
  if(xstr.empty() || ystr.empty() || id.empty()) {
    reportRunWarning("Bad SWIMMER_ALERT: " + val);
    return;
  }

  // SWIMMER_ALERT is sent every 15 sec; skip if we already rescued this one
  if(m_rescued_ids.count(id))
    return;

  double x = atof(xstr.c_str());
  double y = atof(ystr.c_str());

  // Store the swimmer's position keyed by id (overwrites if re-alerted)
  XYPoint pt(x, y);
  pt.set_label(id);
  m_swimmers[id] = pt;

  // Path changed — recompute and publish updated waypoints
  postUpdatedPath();
}

//---------------------------------------------------------
// Procedure: handleFoundSwimmer()
//   Parses: id=01, finder=abe
//   Called when the shoreside confirms a swimmer has been rescued
//   (by this vehicle or another). Removes from the active list
//   so we don't route back to that location.

void GenRescue::handleFoundSwimmer(const string& val)
{
  string id = tokStringParse(val, "id", ',', '=');
  if(id.empty())
    return;

  m_rescued_ids.insert(id);
  m_swimmers.erase(id);

  if(id == m_committed_id)
    m_has_committed_target = false;

  postUpdatedPath();
}

//---------------------------------------------------------
// Procedure: OnConnectToServer()

bool GenRescue::OnConnectToServer()
{
  registerVariables();
  return(true);
}

//---------------------------------------------------------
// Procedure: Iterate()
//   Called at AppTick rate. Checks whether the vehicle has
//   come within visit_radius of any active swimmer and, if so,
//   marks them as rescued locally without waiting for the shore
//   to confirm via FOUND_SWIMMER.

bool GenRescue::Iterate()
{
  AppCastingMOOSApp::Iterate();

  bool changed = false;
  map<string, XYPoint>::iterator it = m_swimmers.begin();
  while(it != m_swimmers.end()) {
    // Straight-line distance from vehicle to this swimmer
    double dist = hypot(it->second.x() - m_nav_x, it->second.y() - m_nav_y);
    if(dist <= m_visit_radius) {
      if(it->first == m_committed_id)
        m_has_committed_target = false;
      m_rescued_ids.insert(it->first);
      it = m_swimmers.erase(it);
      changed = true;
    } else {
      ++it;
    }
  }

  // Only re-post path if something actually changed this tick
  if(changed)
    postUpdatedPath();

  AppCastingMOOSApp::PostReport();
  return(true);
}

//---------------------------------------------------------
// Procedure: OnStartUp()
//   Reads the configuration block from the .moos file.
//   Supported params: v_name, visit_radius

bool GenRescue::OnStartUp()
{
  AppCastingMOOSApp::OnStartUp();

  STRING_LIST sParams;
  m_MissionReader.EnableVerbatimQuoting(false);
  if(!m_MissionReader.GetConfiguration(GetAppName(), sParams))
    reportConfigWarning("No config block found for " + GetAppName());

  STRING_LIST::iterator p;
  for(p = sParams.begin(); p != sParams.end(); p++) {
    string orig  = *p;
    string line  = *p;
    string param = tolower(biteStringX(line, '=')); // param name (left of '=')
    string value = line;                            // value (right of '=')

    bool handled = false;

    if(param == "v_name") {
      m_vname  = value;
      handled  = true;
    } else if(param == "visit_radius") {
      m_visit_radius = atof(value.c_str());
      handled = true;
    } else if(param == "heuristic") {
      m_heuristic = value;
      handled = true;
    } else if(param == "tmate") {
      m_tmate = value;
      handled = true;
    } else if(param == "lookahead_dist") {
      m_lookahead_dist = atof(value.c_str());
      handled = true;
    } else if(param == "cluster_radius") {
      m_cluster_radius = atof(value.c_str());
      handled = true;
    }

    if(!handled)
      reportUnhandledConfigWarning(orig);
  }

  registerVariables();
  return(true);
}

//---------------------------------------------------------
// Procedure: registerVariables()
//   Tells the MOOSDB which variables this app wants to receive.

void GenRescue::registerVariables()
{
  AppCastingMOOSApp::RegisterVariables();
  Register("SWIMMER_ALERT", 0);
  Register("FOUND_SWIMMER",  0);
  Register("NAV_X",          0);
  Register("NAV_Y",          0);
  if(m_heuristic == "pinky" || m_heuristic == "secondstring")
    Register("NODE_REPORT",  0);
}

//---------------------------------------------------------
// Procedure: handleNodeReport()
//   Parses a NODE_REPORT and updates the opponent state if
//   the report is from the configured tmate vehicle.

void GenRescue::handleNodeReport(const string& val)
{
  if(m_tmate.empty())
    return;
  string name = tokStringParse(val, "NAME", ',', '=');
  if(name != m_tmate)
    return;

  string xstr   = tokStringParse(val, "X",   ',', '=');
  string ystr   = tokStringParse(val, "Y",   ',', '=');
  string hdgstr = tokStringParse(val, "HDG", ',', '=');
  if(xstr.empty() || ystr.empty() || hdgstr.empty())
    return;

  m_opp_x   = atof(xstr.c_str());
  m_opp_y   = atof(ystr.c_str());
  m_opp_hdg = atof(hdgstr.c_str());
  m_opp_known = true;
}

//---------------------------------------------------------
// Procedure: selectPinkyTarget()
//   Projects the opponent forward by lookahead_dist, finds
//   the swimmer nearest that point, and returns it as the
//   next target if we can arrive before the opponent.
//   Falls back to the swimmer nearest us (greedy) otherwise.

XYPoint GenRescue::selectPinkyTarget()
{
  // Compute the Pinky lookahead point
  double lax = m_nav_x, lay = m_nav_y;
  if(m_opp_known) {
    double hdg_rad = m_opp_hdg * M_PI / 180.0;
    lax = m_opp_x + m_lookahead_dist * sin(hdg_rad);
    lay = m_opp_y + m_lookahead_dist * cos(hdg_rad);
  }

  XYPoint pinky_pt, greedy_pt;
  double pinky_la_dist = 1e9, greedy_my_dist = 1e9;

  for(map<string, XYPoint>::iterator it = m_swimmers.begin();
      it != m_swimmers.end(); ++it) {
    double sx = it->second.x(), sy = it->second.y();

    double la_dist = hypot(lax - sx, lay - sy);
    if(la_dist < pinky_la_dist) {
      pinky_la_dist = la_dist;
      pinky_pt      = it->second;
    }

    double my_dist = hypot(m_nav_x - sx, m_nav_y - sy);
    if(my_dist < greedy_my_dist) {
      greedy_my_dist = my_dist;
      greedy_pt      = it->second;
    }
  }

  // Use Pinky target only if we can beat the opponent there
  if(m_opp_known) {
    double opp_dist = hypot(m_opp_x - pinky_pt.x(), m_opp_y - pinky_pt.y());
    double my_dist  = hypot(m_nav_x  - pinky_pt.x(), m_nav_y  - pinky_pt.y());
    if(my_dist < opp_dist)
      return pinky_pt;
  }

  return greedy_pt;
}


//---------------------------------------------------------
// Procedure: selectClusterDenialTarget()
//   Scores each swimmer by how many others are within
//   cluster_radius. Returns the id of the swimmer sitting
//   in the densest neighbourhood — the highest-value cluster
//   to race for and deny the opponent.

string GenRescue::selectClusterDenialTarget()
{
  string best_id    = "";
  int    best_count = -1;

  for(map<string, XYPoint>::iterator it = m_swimmers.begin();
      it != m_swimmers.end(); ++it) {
    int count = 0;
    for(map<string, XYPoint>::iterator jt = m_swimmers.begin();
        jt != m_swimmers.end(); ++jt) {
      if(it->first == jt->first) continue;
      double d = hypot(it->second.x() - jt->second.x(),
                       it->second.y() - jt->second.y());
      if(d <= m_cluster_radius)
        count++;
    }
    // Tiebreak: if equal density, prefer the swimmer closer to us
    if(count > best_count ||
       (count == best_count && best_id != "" &&
        hypot(m_nav_x - it->second.x(), m_nav_y - it->second.y()) <
        hypot(m_nav_x - m_swimmers[best_id].x(), m_nav_y - m_swimmers[best_id].y()))) {
      best_count = count;
      best_id    = it->first;
    }
  }

  return best_id;
}

//---------------------------------------------------------
// Procedure: buildAdversarialPath()
//   Puts the chosen first target at the front, then continues
//   greedy nearest-neighbor FROM that target through the rest.

XYSegList GenRescue::buildAdversarialPath(XYPoint first)
{
  vector<XYPoint> remaining;
  for(map<string, XYPoint>::iterator it = m_swimmers.begin();
      it != m_swimmers.end(); ++it) {
    if(hypot(it->second.x() - first.x(), it->second.y() - first.y()) > 0.01)
      remaining.push_back(it->second);
  }

  XYSegList seglist;
  seglist.add_vertex(first.x(), first.y());
  XYSegList rest = buildGreedyPath(remaining, first.x(), first.y());
  for(unsigned int i = 0; i < rest.size(); i++)
    seglist.add_vertex(rest.get_vx(i), rest.get_vy(i));
  return seglist;
}

//---------------------------------------------------------
// Procedure: postUpdatedPath()

void GenRescue::postUpdatedPath()
{
  if(m_swimmers.empty()) {
    Notify("SURVEY_UPDATE", "points=0,0");
    Notify("RETURN", "true");
    return;
  }

  if(m_heuristic == "pinky") {
    XYSegList seglist = buildAdversarialPath(selectPinkyTarget());
    Notify("SURVEY_UPDATE", "points=" + seglist.get_spec());
    return;
  }

  if(m_heuristic == "secondstring") {
    if(!m_has_committed_target || !m_swimmers.count(m_committed_id)) {
      vector<pair<double, string>> by_dist;
      for(map<string, XYPoint>::iterator it = m_swimmers.begin();
          it != m_swimmers.end(); ++it) {
        double d = hypot(m_nav_x - it->second.x(), m_nav_y - it->second.y());
        by_dist.push_back(make_pair(d, it->first));
      }
      sort(by_dist.begin(), by_dist.end());
      unsigned int idx = by_dist.size() / 3;
      if(idx >= by_dist.size()) idx = by_dist.size() - 1;
      m_committed_id         = by_dist[idx].second;
      m_has_committed_target = true;
    }
    XYSegList seglist = buildAdversarialPath(m_swimmers[m_committed_id]);
    Notify("SURVEY_UPDATE", "points=" + seglist.get_spec());
    return;
  }

  if(m_heuristic == "clusterdenial") {
    if(!m_has_committed_target || !m_swimmers.count(m_committed_id)) {
      m_committed_id         = selectClusterDenialTarget();
      m_has_committed_target = true;
    }
    XYSegList seglist = buildAdversarialPath(m_swimmers[m_committed_id]);
    Notify("SURVEY_UPDATE", "points=" + seglist.get_spec());
    return;
  }

  // Default: greedy nearest-neighbor tour
  vector<XYPoint> pts;
  for(map<string, XYPoint>::iterator it = m_swimmers.begin();
      it != m_swimmers.end(); ++it)
    pts.push_back(it->second);
  Notify("SURVEY_UPDATE", "points=" + buildGreedyPath(pts).get_spec());
}

//---------------------------------------------------------
// Procedure: buildGreedyPath()

XYSegList GenRescue::buildGreedyPath(vector<XYPoint> points)
{
  return buildGreedyPath(points, m_nav_x, m_nav_y);
}

XYSegList GenRescue::buildGreedyPath(vector<XYPoint> points, double cx, double cy)
{
  XYSegList seglist;
  vector<XYPoint> remaining = points;

  while(!remaining.empty()) {
    unsigned int nearest_idx = 0;
    double min_dist = hypot(remaining[0].x() - cx, remaining[0].y() - cy);

    for(unsigned int i = 1; i < remaining.size(); i++) {
      double dist = hypot(remaining[i].x() - cx, remaining[i].y() - cy);
      if(dist < min_dist) {
        min_dist    = dist;
        nearest_idx = i;
      }
    }

    seglist.add_vertex(remaining[nearest_idx].x(), remaining[nearest_idx].y());
    cx = remaining[nearest_idx].x();
    cy = remaining[nearest_idx].y();
    remaining.erase(remaining.begin() + nearest_idx);
  }

  return(seglist);
}

//------------------------------------------------------------
// Procedure: buildReport()
//   Populates the appcast panel shown in uMAC / pMarineViewer.

bool GenRescue::buildReport()
{
  m_msgs << "==========================================" << endl;
  m_msgs << "pGenRescue Status                         " << endl;
  m_msgs << "==========================================" << endl;

  m_msgs << "Vehicle      : " << m_vname << endl;
  m_msgs << "Nav position : (" << m_nav_x << ", " << m_nav_y << ")" << endl;
  m_msgs << "Visit radius : " << m_visit_radius << "m" << endl;
  m_msgs << endl;
  m_msgs << "Active swimmers  : " << m_swimmers.size()    << endl;
  m_msgs << "Rescued swimmers : " << m_rescued_ids.size() << endl;

  // List each swimmer still needing rescue with its coordinates
  if(!m_swimmers.empty()) {
    m_msgs << endl;
    m_msgs << "-- Remaining Swimmers --" << endl;
    for(map<string, XYPoint>::iterator it = m_swimmers.begin();
        it != m_swimmers.end(); ++it) {
      m_msgs << "  id=" << it->first
             << "  x=" << it->second.x()
             << "  y=" << it->second.y() << endl;
    }
  }

  return(true);
}
