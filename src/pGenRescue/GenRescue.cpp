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
  m_nav_x        = 0;   // vehicle's current x position (from NAV_X)
  m_nav_y        = 0;   // vehicle's current y position (from NAV_Y)
  m_vname        = "";  // vehicle name, set from config block
  m_visit_radius = 5;   // how close (meters) before a swimmer counts as rescued
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

    if(key == "NAV_X")                        // update own x position
      m_nav_x = msg.GetDouble();
    else if(key == "NAV_Y")                   // update own y position
      m_nav_y = msg.GetDouble();
    else if(key == "SWIMMER_ALERT")           // new swimmer location from shore
      handleSwimmerAlert(msg.GetString());
    else if(key == "FOUND_SWIMMER")           // swimmer rescued (by any vehicle)
      handleFoundSwimmer(msg.GetString());
    else if(key != "APPCAST_REQ")             // ignore appcasting requests silently
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

  // Add to the rescued set so future SWIMMER_ALERTs for this id are ignored
  m_rescued_ids.insert(id);
  // Remove from the active swimmer map
  m_swimmers.erase(id);

  // Path changed — recompute and publish updated waypoints
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
      // Close enough — mark rescued and remove from active map
      m_rescued_ids.insert(it->first);
      it = m_swimmers.erase(it); // erase returns the next valid iterator
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

    if(param == "v_name") {           // vehicle name from the .moos config
      m_vname  = value;
      handled  = true;
    } else if(param == "visit_radius") { // override default rescue radius
      m_visit_radius = atof(value.c_str());
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
  Register("SWIMMER_ALERT", 0); // swimmer location alerts from shore
  Register("FOUND_SWIMMER",  0); // rescue confirmations from shore
  Register("NAV_X",          0); // own x position
  Register("NAV_Y",          0); // own y position
}

//---------------------------------------------------------
// Procedure: postUpdatedPath()
//   Converts the current active swimmer map into a greedy
//   waypoint path and posts it as SURVEY_UPDATE so that
//   BHV_Waypoint can execute it immediately.

void GenRescue::postUpdatedPath()
{
  if(m_swimmers.empty()) {
    // All swimmers accounted for — send dummy point and trigger return
    Notify("SURVEY_UPDATE", "points=0,0");
    Notify("RETURN", "true");
    return;
  }

  // Collect all unrescued swimmer positions into a vector for path building
  vector<XYPoint> pts;
  for(map<string, XYPoint>::iterator it = m_swimmers.begin();
      it != m_swimmers.end(); ++it)
    pts.push_back(it->second);

  // Build the greedy path and convert to BHV_Waypoint update string
  XYSegList seglist = buildGreedyPath(pts);
  string spec = seglist.get_spec();          // e.g. "34,85:12,40:..."
  Notify("SURVEY_UPDATE", "points=" + spec); // drives BHV_Waypoint via updates=
}

//---------------------------------------------------------
// Procedure: buildGreedyPath()
//   Builds a nearest-neighbor tour through the given points,
//   starting from the vehicle's current position. Not globally
//   optimal but fast and good enough for live replanning.

XYSegList GenRescue::buildGreedyPath(vector<XYPoint> points)
{
  XYSegList seglist;
  vector<XYPoint> remaining = points; // working copy — we remove as we go

  // Start search from the vehicle's current position
  double cx = m_nav_x;
  double cy = m_nav_y;

  while(!remaining.empty()) {
    // Find the nearest unvisited point to the current position (cx, cy)
    unsigned int nearest_idx = 0;
    double min_dist = hypot(remaining[0].x() - cx, remaining[0].y() - cy);

    for(unsigned int i = 1; i < remaining.size(); i++) {
      double dist = hypot(remaining[i].x() - cx, remaining[i].y() - cy);
      if(dist < min_dist) {
        min_dist    = dist;
        nearest_idx = i;
      }
    }

    // Commit that point as the next waypoint and advance the search origin
    seglist.add_vertex(remaining[nearest_idx].x(), remaining[nearest_idx].y());
    cx = remaining[nearest_idx].x();
    cy = remaining[nearest_idx].y();
    remaining.erase(remaining.begin() + nearest_idx); // remove so we don't revisit
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
