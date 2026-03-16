/****************************************************************/
/*   NAME: DerekOSullivan                                       */
/*   ORGN: MIT, Cambridge MA                                    */
/*   FILE: GenPath_Info.cpp                                     */
/*   DATE: 13mar26                                              */
/****************************************************************/

#include <cstdlib>
#include <iostream>
#include "GenPath_Info.h"
#include "ColorParse.h"
#include "ReleaseInfo.h"

using namespace std;

//----------------------------------------------------------------
// Procedure: showSynopsis

void showSynopsis()
{
  blk("SYNOPSIS:                                                       ");
  blk("------------------------------------                            ");
  blk("  pGenPath collects VISIT_POINT messages, computes a greedy    ");
  blk("  nearest-neighbor path from the vehicle's current position,   ");
  blk("  and publishes waypoint updates for pHelmIvP.                 ");
}

//----------------------------------------------------------------
// Procedure: showHelpAndExit

void showHelpAndExit()
{
  blk("                                                                ");
  blu("=============================================================== ");
  blu("Usage: pGenPath file.moos [OPTIONS]                             ");
  blu("=============================================================== ");
  blk("                                                                ");
  showSynopsis();
  blk("                                                                ");
  blk("Options:                                                        ");
  mag("  --alias","=<ProcessName>                                      ");
  blk("      Launch pGenPath with the given process name               ");
  blk("      rather than pGenPath.                                     ");
  mag("  --example, -e                                                 ");
  blk("      Display example MOOS configuration block.                 ");
  mag("  --help, -h                                                    ");
  blk("      Display this help message.                                ");
  mag("  --interface, -i                                               ");
  blk("      Display MOOS publications and subscriptions.              ");
  mag("  --version,-v                                                  ");
  blk("      Display the release version of pGenPath.                  ");
  blk("                                                                ");
  exit(0);
}

//----------------------------------------------------------------
// Procedure: showExampleConfigAndExit

void showExampleConfigAndExit()
{
  blk("                                                                ");
  blu("=============================================================== ");
  blu("pGenPath Example MOOS Configuration                             ");
  blu("=============================================================== ");
  blk("                                                                ");
  blk("ProcessConfig = pGenPath                                        ");
  blk("{                                                               ");
  blk("  AppTick   = 4                                                 ");
  blk("  CommsTick = 4                                                 ");
  blk("}                                                               ");
  blk("                                                                ");
  exit(0);
}

//----------------------------------------------------------------
// Procedure: showInterfaceAndExit

void showInterfaceAndExit()
{
  blk("                                                                ");
  blu("=============================================================== ");
  blu("pGenPath INTERFACE                                              ");
  blu("=============================================================== ");
  blk("                                                                ");
  showSynopsis();
  blk("                                                                ");
  blk("SUBSCRIPTIONS:                                                  ");
  blk("------------------------------------                            ");
  blk("  VISIT_POINT = firstpoint                                      ");
  blk("  VISIT_POINT = x=<val>,y=<val>,id=<val>                       ");
  blk("  VISIT_POINT = lastpoint                                       ");
  blk("  NAV_X       = <double>                                        ");
  blk("  NAV_Y       = <double>                                        ");
  blk("                                                                ");
  blk("PUBLICATIONS:                                                   ");
  blk("------------------------------------                            ");
  blk("  GENPATH_UPDATES  = points=<seglist_spec>                      ");
  blk("  GENPATH_SEGLIST  = <seglist_spec>                             ");
  blk("                                                                ");
  exit(0);
}

//----------------------------------------------------------------
// Procedure: showReleaseInfoAndExit

void showReleaseInfoAndExit()
{
  showReleaseInfo("pGenPath", "gpl");
  exit(0);
}
