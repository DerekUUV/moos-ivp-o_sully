/************************************************************/
/*    NAME: DerekOSullivan                                  */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: PointAssign.cpp                                 */
/*    DATE: 10mar26                                         */
/************************************************************/

#include <iterator>
#include "MBUtils.h"
#include "ACTable.h"
#include "PointAssign.h"
#include "XYFormatUtilsPoint.h"
using namespace std;

//---------------------------------------------------------
// Constructor()

PointAssign::PointAssign()
{
  m_assign_by_region = true;  // hard coded for now, can be set in .moos config
  m_next_index       = 0;     // tracks which vehicle gets the next point (round-robin)
  m_mid_x            = 0;     // default the value, pre-fill to zero
  m_mid_x_computed   = false; // status on computing mid before divying up points
  m_timer_started    = false; // tracks if timer has been unpaused already
  m_henry_ready      = false; // hard code for name
  m_gilda_ready      = false; // hard code for name

  m_vnames.push_back("henry"); // add vehicles to the list
  m_vnames.push_back("gilda");
}

//---------------------------------------------------------
// Destructor

PointAssign::~PointAssign()
{
  // nothing required here
}

//---------------------------------------------------------
// Procedure: OnNewMail()
//   Receives VISIT_POINT messages from uTimerScript and
//   NODE_REPORT messages to know when vehicles are connected

bool PointAssign::OnNewMail(MOOSMSG_LIST &NewMail)
{
  AppCastingMOOSApp::OnNewMail(NewMail);

  MOOSMSG_LIST::iterator p;
  for(p=NewMail.begin(); p!=NewMail.end(); p++) {
    CMOOSMsg &msg = *p;
    string key = msg.GetKey();

    if(key == "VISIT_POINT") {
      m_visit_points.push_back(msg.GetString()); // store raw point strings for processing in Iterate()
    }

    if(key == "NODE_REPORT") {
      string sval = msg.GetString();
      string name = tokStringParse(sval, "NAME", ',', '='); // pull vehicle name out of the report

      if(name == "henry")
        m_henry_ready = true; // henry has connected
      if(name == "gilda")
        m_gilda_ready = true; // gilda has connected
    }
  }

  return(true);
}

//---------------------------------------------------------
// Procedure: OnConnectToServer()

bool PointAssign::OnConnectToServer()
{
  registerVariables();
  return(true);
  // nothing added here
}

//---------------------------------------------------------
// Procedure: Iterate()

bool PointAssign::Iterate()
{
  AppCastingMOOSApp::Iterate();

  // wait until both vehicles have sent a node report before starting the timer
  if(!m_timer_started && m_henry_ready && m_gilda_ready) {
    Notify("UTIMER_PAUSE", "false"); // unpause timer so points start
    m_timer_started = true;
  }

  // compute the mid line before distributing points

  if(m_assign_by_region && !m_mid_x_computed) {
    bool has_lastpoint = false; // check the full batch has arrived before computing
    for(unsigned int j=0; j<m_visit_points.size(); j++) {
      if(m_visit_points[j] == "lastpoint") { has_lastpoint = true; break; }
    }
    if(has_lastpoint) {
      double sum   = 0;
      int    count = 0;
      for(unsigned int j=0; j<m_visit_points.size(); j++) {
        string sval = m_visit_points[j];
        if(sval != "firstpoint" && sval != "lastpoint") {
          string temp  = sval;
          string xpart = biteStringX(temp, ','); // pull the x=val token
          biteStringX(xpart, '=');               // strip the "x=" prefix to get the number
          double x = atof(xpart.c_str());
          sum += x;
          count++;
        }
      }
      if(count > 0) {
        m_mid_x          = sum / (double)count; // average x is the dividing line
        m_mid_x_computed = true;
      }
    }
  }

  // process every point buffered since last Iterate()
  for(unsigned int j=0; j<m_visit_points.size(); j++) {
    string sval = m_visit_points[j];

    if(sval == "firstpoint") {
      reportEvent("First point marker received");
      m_next_index = 0; // reset round-robin index at the start of each batch

      // forward firstpoint to every vehicle so they know a batch is starting
      for(unsigned int i=0; i<m_vnames.size(); i++) {
        string var = "VISIT_POINT_" + toupper(m_vnames[i]);
        Notify(var, "firstpoint");
      }
    }
    else if(sval == "lastpoint") {
      reportEvent("Last point marker received");

      // forward lastpoint to every vehicle so they know the batch is done
      for(unsigned int i=0; i<m_vnames.size(); i++) {
        string var = "VISIT_POINT_" + toupper(m_vnames[i]);
        Notify(var, "lastpoint");
      }
    }
    else {
      string vname; // name of the vehicle this point will be sent to
      string temp   = sval;
      string xpart  = biteStringX(temp, ',');
      string ypart  = biteStringX(temp, ',');
      string idpart = temp;

      biteStringX(xpart,  '='); // strip key prefix, leave value
      biteStringX(ypart,  '=');
      biteStringX(idpart, '=');

      double x = atof(xpart.c_str());
      double y = atof(ypart.c_str());

      if(m_assign_by_region) {
        // split by x position relative to the computed mid x
        if(x < m_mid_x)
          vname = m_vnames[0]; // left region goes to henry
        else
          vname = m_vnames[1]; // right region goes to gilda
      }
      else {
        // round-robin: alternate points between vehicles
        vname = m_vnames[m_next_index % m_vnames.size()];
        m_next_index++;
      }

      string var = "VISIT_POINT_" + toupper(vname); // build the per-vehicle variable name
      Notify(var, sval);                             // send point to that vehicle

      // show the point on pMarineViewer in the vehicle's color
      if(vname == "henry")
        postViewPoint(x, y, idpart, "yellow");
      else
        postViewPoint(x, y, idpart, "cyan");

      reportEvent("Sent to " + var + ": " + sval);
    }
  }

  m_visit_points.clear(); // done processing this batch, clear for next Iterate()

  AppCastingMOOSApp::PostReport();
  return(true);
}

//---------------------------------------------------------
// Procedure: OnStartUp()
//   Reads config from .moos file, currently only assign_by_region

bool PointAssign::OnStartUp()
{
  AppCastingMOOSApp::OnStartUp();

  STRING_LIST sParams;
  if(!m_MissionReader.GetConfiguration(GetAppName(), sParams))
    reportConfigWarning("No config block found for " + GetAppName());

  STRING_LIST::iterator p;
  for(p=sParams.begin(); p!=sParams.end(); p++) {
    string line  = *p;
    string param = biteStringX(line, '='); // split on = to get param name
    string value = line;                   // remainder is the value

    if(param == "assign_by_region") {
      m_assign_by_region = (value == "true"); // set assignment mode from config
    }
  }

  registerVariables();
  return(true);
}

//---------------------------------------------------------
// Procedure: registerVariables()
//   Subscribe to the variables this app needs from the MOOSDB

void PointAssign::registerVariables()
{
  AppCastingMOOSApp::RegisterVariables();
  Register("VISIT_POINT", 0); // random points from uTimerScript
  Register("NODE_REPORT",  0); // vehicle connection status
}

//---------------------------------------------------------
// Procedure: postViewPoint()
//   Publish a colored marker to pMarineViewer for a given point

void PointAssign::postViewPoint(double x, double y, string label, string color)
{
  XYPoint point(x, y);
  point.set_label(label);
  point.set_color("vertex", color); // color shows which vehicle owns this point
  point.set_param("vertex_size", "4");

  string spec = point.get_spec(); // convert to MOOS string format
  Notify("VIEW_POINT", spec);
}

//---------------------------------------------------------
// Procedure: buildReport()
//   AppCast status display shown in pMarineViewer

bool PointAssign::buildReport()
{
  m_msgs << "Henry ready     : " << (m_henry_ready ? "YES" : "NO") << endl;
  m_msgs << "Gilda ready     : " << (m_gilda_ready ? "YES" : "NO") << endl;
  m_msgs << "Timer started   : " << (m_timer_started ? "YES" : "NO") << endl;
  m_msgs << "Assign by region: " << (m_assign_by_region ? "YES" : "NO") << endl;
  m_msgs << "Mid X computed  : " << (m_mid_x_computed ? "YES" : "NO") << endl;
  m_msgs << "Mid X value     : " << m_mid_x << endl;

  return(true);
}
