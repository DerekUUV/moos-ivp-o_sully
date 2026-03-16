/************************************************************/
/*    NAME: DerekOSullivan                                  */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: GenPath.cpp                                     */
/*    DATE: 13mar26                                         */
/************************************************************/

#include <iterator>
#include <cmath>
#include "MBUtils.h"
#include "ACTable.h"
#include "GenPath.h"
#include "XYFormatUtilsPoint.h"

using namespace std;

//---------------------------------------------------------
// Constructor()

GenPath::GenPath()
{
  m_receiving       = false;
  m_points_received = false;
  m_path_posted     = false;
  m_traverse_posted = false;
  m_nav_x           = 0;
  m_nav_y           = 0;
  m_vname           = "";
}

//---------------------------------------------------------
// Destructor

GenPath::~GenPath()
{
}

//---------------------------------------------------------
// Procedure: OnNewMail()

bool GenPath::OnNewMail(MOOSMSG_LIST &NewMail)
{
  AppCastingMOOSApp::OnNewMail(NewMail);

  MOOSMSG_LIST::iterator p;
  for (p = NewMail.begin(); p != NewMail.end(); p++)
  {
    CMOOSMsg &msg = *p;
    string key    = msg.GetKey();

    if (key == "NAV_X")
      m_nav_x = msg.GetDouble();

    else if (key == "NAV_Y")
      m_nav_y = msg.GetDouble();

    else if (key == "VISIT_POINT")
    {
      string val = msg.GetString();

      if (val == "firstpoint")
      {
        m_vector_points.clear();  // fresh start
        m_receiving       = true;
        m_points_received = false;
        m_path_posted     = false;
      }
      else if (val == "lastpoint")
      {
        m_receiving       = false;
        m_points_received = true;  // ready to build path
      }
      else if (m_receiving)
      {
        XYPoint pt = string2Point(val);
        string id  = tokStringParse(val, "id", ',', '=');
        pt.set_label(id);
        m_vector_points.push_back(pt);
      }
    }
    else if (key != "APPCAST_REQ")
      reportRunWarning("Unhandled Mail: " + key);
  }
  return (true);
}

//---------------------------------------------------------
// Procedure: OnConnectToServer()

bool GenPath::OnConnectToServer()
{
  registerVariables();
  return (true);
}

//---------------------------------------------------------
// Procedure: Iterate()

bool GenPath::Iterate()
{
  AppCastingMOOSApp::Iterate();

  // Once all points received and path not yet posted, compute and publish
  if (m_points_received && !m_path_posted)
  {
    if (!m_vector_points.empty()) {
      XYSegList seglist = buildGreedyPath();
      string update_str = "points = " + seglist.get_spec();
      Notify("GENPATH_UPDATES", update_str);
      Notify("GENPATH_SEGLIST", seglist.get_spec());  // for pMarineViewer
    }
    Notify("TRAVERSE", "true");  // always signal done, even if no points assigned
    m_path_posted = true;
  }

  AppCastingMOOSApp::PostReport();
  return (true);
}

//---------------------------------------------------------
// Procedure: OnStartUp()

bool GenPath::OnStartUp()
{
  AppCastingMOOSApp::OnStartUp();

  STRING_LIST sParams;
  m_MissionReader.EnableVerbatimQuoting(false);
  if (!m_MissionReader.GetConfiguration(GetAppName(), sParams))
    reportConfigWarning("No config block found for " + GetAppName());

  STRING_LIST::iterator p;
  for (p = sParams.begin(); p != sParams.end(); p++)
  {
    string orig  = *p;
    string line  = *p;
    string param = tolower(biteStringX(line, '='));
    string value = line;

    bool handled = false;

    if (param == "v_name") {
      m_vname = value;  // vehicle name, used to subscribe to VISIT_POINT_{VNAME}
      handled = true;
    }

    if (!handled)
      reportUnhandledConfigWarning(orig);
  }

  registerVariables();
  return (true);
}

//---------------------------------------------------------
// Procedure: registerVariables()

void GenPath::registerVariables()
{
  AppCastingMOOSApp::RegisterVariables();
  Register("VISIT_POINT", 0);
  Register("NAV_X",       0);
  Register("NAV_Y",       0);
}

//---------------------------------------------------------
// Procedure: buildGreedyPath()
//   Greedy nearest-neighbor tour starting from current position

XYSegList GenPath::buildGreedyPath()
{
  XYSegList seglist;
  vector<XYPoint> remaining = m_vector_points;

  double cx = m_nav_x;
  double cy = m_nav_y;

  while (!remaining.empty())
  {
    // find the nearest unvisited point
    unsigned int nearest_idx = 0;
    double min_dist = hypot(remaining[0].x() - cx, remaining[0].y() - cy);

    for (unsigned int i = 1; i < remaining.size(); i++)
    {
      double dist = hypot(remaining[i].x() - cx, remaining[i].y() - cy);
      if (dist < min_dist)
      {
        min_dist    = dist;
        nearest_idx = i;
      }
    }

    // add nearest point to path and advance current position
    seglist.add_vertex(remaining[nearest_idx].x(), remaining[nearest_idx].y());
    cx = remaining[nearest_idx].x();
    cy = remaining[nearest_idx].y();
    remaining.erase(remaining.begin() + nearest_idx);
  }

  return (seglist);
}

//------------------------------------------------------------
// Procedure: buildReport()

bool GenPath::buildReport()
{
  m_msgs << "==========================================" << endl;
  m_msgs << "pGenPath Status                           " << endl;
  m_msgs << "==========================================" << endl;

  m_msgs << "Nav position : (" << m_nav_x << ", " << m_nav_y << ")" << endl;
  m_msgs << "Points received: " << m_vector_points.size() << endl;
  m_msgs << "Receiving      : " << (m_receiving ? "YES" : "NO") << endl;
  m_msgs << "All received   : " << (m_points_received ? "YES" : "NO") << endl;
  m_msgs << "Path posted    : " << (m_path_posted ? "YES" : "NO") << endl;

  m_msgs << endl;
  m_msgs << "-- Assigned Points --" << endl;
  for (unsigned int i = 0; i < m_vector_points.size(); i++)
  {
    m_msgs << "  [" << i << "] x=" << m_vector_points[i].x()
           << "  y=" << m_vector_points[i].y()
           << "  id=" << m_vector_points[i].get_label() << endl;
  }

  return (true);
}
