/************************************************************/
/*    NAME: DerekOSullivan                                              */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: PointAssign.cpp                                        */
/*    DATE: 10mar26                         */
/************************************************************/

#include <iterator>
#include "MBUtils.h"
#include "ACTable.h"
// added
#include "PointAssign.h"
#include "XYFormatUtilsPoint.h"
using namespace std;

//---------------------------------------------------------
// Constructor()

PointAssign::PointAssign()
{
  assign_by_region = true; //hard coded, will need to have option
  m_points_assigned = false;
  m_median_x = 0; //original value for the middle
}

  
//---------------------------------------------------------
// Destructor
PointAssign::~PointAssign()
{
}
//---------------------------------------------------------
// Procedure: OnNewMail()

bool PointAssign::OnNewMail(MOOSMSG_LIST &NewMail)
{
  AppCastingMOOSApp::OnNewMail(NewMail);

  MOOSMSG_LIST::iterator p;
  for (p = NewMail.begin(); p != NewMail.end(); p++)
  {
    CMOOSMsg &msg = *p;
    string key = msg.GetKey();

#if 0 // Keep these around just for template
    string comm  = msg.GetCommunity();
    double dval  = msg.GetDouble();
    string sval  = msg.GetString(); 
    string msrc  = msg.GetSource();
    double mtime = msg.GetTime();
    bool   mdbl  = msg.IsDouble();
    bool   mstr  = msg.IsString();
#endif

    if (key == "VISIT_POINT")
    {                                                                  // need to grab the info for the VISIT POINT that is required shoreside
      string visitpoint = msg.GetString();                             // visitpoint is the string that is for the .cpp
      if ((visitpoint == "lastpoint") || (visitpoint == "firstpoint")) // if it is the first or last point
      {
        // do nothing for now
      }
      else
      {
        XYPoint pt = string2Point(visitpoint);                  // sends out the XY pont
        string id = tokStringParse(visitpoint, "id", ',', '='); // associates the var 'id' to be the label of data
        pt.set_label(id);                                       // actually labels it
        m_vector_points.push_back(pt);                          // updates and saves the information
      }
    }
    else if (key != "APPCAST_REQ") // handled by AppCastingMOOSApp
      reportRunWarning("Unhandled Mail: " + key);
  }
  return (true);
}
//---------------------------------------------------------
// Procedure: OnConnectToServer()

bool PointAssign::OnConnectToServer()
{
  registerVariables();
  return (true);
}

//---------------------------------------------------------
// Procedure: Iterate()
//            happens AppTick times per second

bool PointAssign::Iterate()
{
  AppCastingMOOSApp::Iterate();

  if (!m_points_assigned && !m_vector_points.empty())
  {
    // compute average x to split region between vehicles, will define here the middle x
    double sum = 0;
    for (unsigned int i = 0; i < m_vector_points.size(); i++)
      sum += m_vector_points.at(i).x();
    m_median_x = sum / m_vector_points.size();

    for (unsigned int i = 0; i < m_vector_vehicles.size(); i++) // for each vehicle it needs to get info
    {
      string v_name = m_vector_vehicles.at(i);       // gets the name of the vehicle at position i
      v_name = toupper(v_name);                      // this will make the received string name above and UPPERCASE
      Notify("VISIT_POINT_" + v_name, "firstpoint"); // this will notif ythe respective ehicle that it is on first point
    }
    //-----
    for (unsigned int i = 0; i < m_vector_points.size(); i++) // this will now read how many positions are
    {
      if (assign_by_region == false)
      {
        XYPoint current_point = m_vector_points.at(i);
        string color = (i % 2 == 0) ? "red" : "yellow";
        Notify("VISIT_POINT_" + toupper(m_vector_vehicles.at(i % 2)), current_point.get_spec());
        postViewPoint(current_point.x(), current_point.y(), current_point.get_label(), color);
      }

      else // this is the logic for dividing
      {
        XYPoint current_point = m_vector_points.at(i);
        if (current_point.x() < m_median_x)
        {
          Notify("VISIT_POINT_" + toupper(m_vector_vehicles.at(0)), current_point.get_spec()); // to first
          postViewPoint(current_point.x(), current_point.y(), current_point.get_label(), "red");
          m_map_vehicle_count[toupper(m_vector_vehicles.at(0))]++;
        }
        else
        {
          Notify("VISIT_POINT_" + toupper(m_vector_vehicles.at(1)), current_point.get_spec()); // to second
          postViewPoint(current_point.x(), current_point.y(), current_point.get_label(), "yellow");
          m_map_vehicle_count[toupper(m_vector_vehicles.at(1))]++;
        }
      }
  }
 

    //----
    for (unsigned int i = 0; i < m_vector_vehicles.size(); i++) // for each vehicle
    {
      string v_name = m_vector_vehicles.at(i);      // gets the name of the vehicle at position i
      v_name = toupper(v_name);                     // this will make the received string name above and UPPERCASE
      Notify("VISIT_POINT_" + v_name, "lastpoint"); // this will notify the respective ehicle that it is on last point
    }
    m_points_assigned = true;
  }

  AppCastingMOOSApp::PostReport();
  return (true);
}

//---------------------------------------------------------
// Procedure: OnStartUp()
//            happens before connection is open

bool PointAssign::OnStartUp()
{
  AppCastingMOOSApp::OnStartUp();

  STRING_LIST sParams;
  m_MissionReader.EnableVerbatimQuoting(false);
  if (!m_MissionReader.GetConfiguration(GetAppName(), sParams))
    reportConfigWarning("No config block found for " + GetAppName());

  STRING_LIST::iterator p;
  for (p = sParams.begin(); p != sParams.end(); p++)
  {
    string orig = *p;
    string line = *p;
    string param = tolower(biteStringX(line, '='));
    string value = line;

    bool handled = false;
    if (param == "v_name")
    {
      handled = true;
      m_vector_vehicles.push_back(value);
    }

    else if (param == "median_x")
    {
      handled = true;
      m_median_x = stod(value);
    }

    else if (param == "assign_by_region")
    {
      handled = true;
      assign_by_region = (tolower(value) == "true");
    }

    if (!handled)
      reportUnhandledConfigWarning(orig);
  }

  registerVariables();
  return (true);
}

//---------------------------------------------------------
// Procedure: registerVariables()

void PointAssign::registerVariables()
{
  AppCastingMOOSApp::RegisterVariables();
  // Register("FOOBAR", 0);
  Register("VISIT_POINT", 0);
}

//------------------------------------------------------------
// Procedure: postViewPoint()

void PointAssign::postViewPoint(double x, double y, string label, string color)
{
  XYPoint point(x, y);
  point.set_label(label);
  point.set_color("vertex", color); //colors based on what vehicleis assigned
  point.set_param("vertex_size", "4");

  string spec = point.get_spec();
  Notify("VIEW_POINT", spec);
}

//------------------------------------------------------------
// Procedure: buildReport()

bool PointAssign::buildReport()
{
  m_msgs << "============================================" << endl;
  m_msgs << "File:                                       " << endl;
  m_msgs << "============================================" << endl;

  m_msgs << "Total points received: " << m_vector_points.size() << endl;
  m_msgs << "Total vehicles: " << m_vector_vehicles.size() << endl;
  m_msgs << "Points assigned: " << (m_points_assigned ? "YES" : "NO") << endl;
  m_msgs << endl;

  map<string, int>::iterator it;
  for (it = m_map_vehicle_count.begin(); it != m_map_vehicle_count.end(); it++)
    m_msgs << "  " << it->first << ": " << it->second << " points" << endl;

  return (true);
}
