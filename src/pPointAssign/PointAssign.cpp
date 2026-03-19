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
  m_assign_by_region = true;  // hard coded for now might add as button
  m_next_index       = 0;     // tracks which vehicle gets the next point (round-robin)
  m_mid_x            = 0;     // default the value, pre-fill to zero
  m_mid_x_computed   = false; // status on computing mid before divying up points
  m_timer_started    = false; // tracks if timer has been unpaused already
  // vehicle names and ready states are loaded from the .moos config block
}

//---------------------------------------------------------
// Destructor

PointAssign::~PointAssign()
{
  // nothing required here
}

//---------------------------------------------------------
// Procedure: OnNewMail()

bool PointAssign::OnNewMail(MOOSMSG_LIST &NewMail)
{
  AppCastingMOOSApp::OnNewMail(NewMail);

  MOOSMSG_LIST::iterator p;
  for(p=NewMail.begin(); p!=NewMail.end(); p++) {
    CMOOSMsg &msg = *p;
    string key = msg.GetKey();

    if(key == "VISIT_POINT") {
      m_visit_points.push_back(msg.GetString()); // store raw point info for processing
    }

    if(key == "NODE_REPORT") {
      string sval = msg.GetString();
      string name = tokStringParse(sval, "NAME", ',', '='); // reads name to deleniate (sp)

      if(m_ready.count(name))
        m_ready[name] = true; // mark this vehicle as alive
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

  // wait until all configured vehicles are up before unpausing timer
  bool all_ready = !m_ready.empty();
  for(map<string,bool>::iterator it=m_ready.begin(); it!=m_ready.end(); ++it)
    if(!it->second) { all_ready = false; break; }

  if(!m_timer_started && all_ready) {
    Notify("UTIMER_PAUSE", "false"); // ready set
    m_timer_started = true; // go
  }

  // computing of the mid line before distributing points bc fancy

  if(m_assign_by_region && !m_mid_x_computed) { //region yes but no mid computed
    bool has_lastpoint = false; // check the full batch has arrived before computing
    for(unsigned int j=0; j<m_visit_points.size(); j++) { // sequential walk through
      if(m_visit_points[j] == "lastpoint") { has_lastpoint = true; break; } //if it is at last point STOP
    }
    if(has_lastpoint) { //this will now seek the final point
      double sum   = 0; //does not care about last
      int    count = 0;
      for(unsigned int j=0; j<m_visit_points.size(); j++) { //
        string sval = m_visit_points[j];
        if(sval != "firstpoint" && sval != "lastpoint") { //if not 1st/last
          string temp  = sval;
          string xpart = biteStringX(temp, ','); // pull the x=val token
          biteStringX(xpart, '=');               // grabs actual x value
          double x = atof(xpart.c_str());
          sum += x; //addinh
          count++;
        }
      }
      if(count > 0) {
        m_mid_x          = sum / (double)count; // average x is the dividing line
        m_mid_x_computed = true; //signals there is a middle point to use
      }
    }
  }

  // process every point buffered since last Iterate()
  for(unsigned int j=0; j<m_visit_points.size(); j++) {
    string sval = m_visit_points[j]; //loop for pushing values

    if(sval == "firstpoint") {
      reportEvent("First point marker received");
      m_next_index = 0; // reset round-robin numba at the start of each batch

      // forward firstpoint to every vehicle so they know a batch is starting
      for(unsigned int i=0; i<m_vnames.size(); i++) {
        string var = "VISIT_POINT_" + toupper(m_vnames[i]); //assignment formatting to send to vic
        Notify(var, "firstpoint"); //hey dawg (e.g. gilda) this is the first thang
      }
    }
    else if(sval == "lastpoint") {
      reportEvent("Last point marker received");

      // forward lastpoint to every vehicle so they know the batch is done
      for(unsigned int i=0; i<m_vnames.size(); i++) {
        string var = "VISIT_POINT_" + toupper(m_vnames[i]);
        Notify(var, "lastpoint"); // hey dawg (e.g. gilda) ya done
      }
    }
    else {
      string vname; // stores data nicely to be sent out
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
          vname = m_vnames[0]; // left region goes to second vehicle in list
        else
          vname = m_vnames[1]; // right region goes to first vehicle in list hopefully the go nicely
      }
      else {
        // round-robin: alternate points between vehicles IF NOT REGIONED
        vname = m_vnames[m_next_index % m_vnames.size()]; 
        m_next_index++; //++ adds a sequencing
      }

      string var = "VISIT_POINT_" + toupper(vname); // build the per-vehicle variable name
      Notify(var, sval);                             // send point to that vehicle

      // show the point on pMarineViewer in the vehicle's color (cycles through list)
      vector<string> colors = {"cyan", "yellow", "orange", "green", "red"};
      int vidx = 0;
      for(unsigned int i=0; i<m_vnames.size(); i++)
        if(m_vnames[i] == vname) { vidx = i; break; }
      postViewPoint(x, y, idpart, colors[vidx % colors.size()]);

      reportEvent("Sent to " + var + ": " + sval); //info goes out to VEHICLE + x,y,id DATA
    }
  }

  m_visit_points.clear(); 

  AppCastingMOOSApp::PostReport();
  return(true);
}

//---------------------------------------------------------
// Procedure: OnStartUp()

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
      m_assign_by_region = (value == "true"); // set assignment mode
    }
    else if(param == "v_name") {
      string vname = tolower(value); // NODE_REPORT names are lowercase
      m_vnames.push_back(vname);
      m_ready[vname] = false;
    }
  }

  if(m_vnames.empty())
    reportConfigWarning("No v_name entries found — add v_name=<name> to config block");

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
  for(map<string,bool>::iterator it=m_ready.begin(); it!=m_ready.end(); ++it)
    m_msgs << it->first << " ready: " << (it->second ? "YES" : "NO") << endl;
  m_msgs << "Timer started   : " << (m_timer_started ? "YES" : "NO") << endl;
  m_msgs << "Assign by region: " << (m_assign_by_region ? "YES" : "NO") << endl;
  m_msgs << "Mid X computed  : " << (m_mid_x_computed ? "YES" : "NO") << endl;
  m_msgs << "Mid X value     : " << m_mid_x << endl;

  return(true);

}
