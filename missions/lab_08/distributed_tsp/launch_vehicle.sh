#!/bin/bash 
#----------------------------------------------------------
#  Script: launch.sh
#  LastEd: Apr 11th 2021
#----------------------------------------------------------
#  Part 1: Set global var defaults
#----------------------------------------------------------
TIME_WARP=10
JUST_MAKE="no"

VNAME1="gilda"  
VNAME2="henry"  
START_POS1="0,0"    
START_POS2="80,0"   
LOITER_POS1="x=0,y=-75"
LOITER_POS2="x=125,y=-50"
IP_ADDR="localhost"
MOOS_PORT="9001"
PSHARE_PORT="9201"
SHORE_IP="localhost"
SHORE_PSHARE="9200"

#-----------------------------------------------------------
#  Part 2: Check for and handle command-line arguments
#-----------------------------------------------------------
for ARGI; do
    if [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ]; then
	echo "launch.sh [SWITCHES] [time_warp] "
	echo "  --just_make, -j    " 
	echo "  --help, -h         " 
	exit 0;
    elif [ "${ARGI//[^0-9]/}" = "$ARGI" -a "$TIME_WARP" = 1 ]; then 
        TIME_WARP=$ARGI
    elif [ "${ARGI}" = "--just_build" -o "${ARGI}" = "-j" ]; then
	JUST_MAKE="yes"
    elif [ "${ARGI:0:5}" = "--ip=" ]; then
        IP_ADDR="${ARGI#--ip=*}"
    elif [ "${ARGI:0:7}" = "--mport" ]; then
	    MOOS_PORT="${ARGI#--mport=*}"
    elif [ "${ARGI:0:9}" = "--pshare=" ]; then
        PSHARE_PORT="${ARGI#--pshare=*}"
    elif [ "${ARGI:0:8}" = "--shore=" ]; then
        SHORE_IP="${ARGI#--shore=*}"
    elif [ "${ARGI:0:15}" = "--shore_pshare=" ]; then
        SHORE_PSHARE="${ARGI#--shore_pshare=*}"
    else 
	echo "launch.sh Bad Arg:" $ARGI " Exit code 1."
	exit 0
    fi
done

#-----------------------------------------------------------
#  Part 3: Create the .moos and .bhv files. 
#-----------------------------------------------------------
nsplug meta_vehicle.moos targ_$VNAME1.moos -f WARP=$TIME_WARP \
       VNAME=$VNAME1        START_POS=$START_POS1             \
       VPORT=$MOOS_PORT         PSHARE_PORT=$PSHARE_PORT                \
       VTYPE="kayak"        SHORE_IP=$SHORE_IP              \
       SHORE_PSHARE=$SHORE_PSHARE  IP_ADDR=$IP_ADDR


nsplug meta_vehicle.moos targ_$VNAME2.moos -f WARP=$TIME_WARP \
       VNAME=$VNAME2        START_POS=$START_POS2             \
       VPORT="9002"         PSHARE_PORT="9202"                \
       VTYPE="kayak"        SHORE_IP=$SHORE_IP               \
       SHORE_PSHARE=$SHORE_PSHARE  IP_ADDR=$IP_ADDR


nsplug meta_vehicle.bhv targ_$VNAME1.bhv -f VNAME=$VNAME1     \
       START_POS=$START_POS1 LOITER_POS=$LOITER_POS1       

nsplug meta_vehicle.bhv targ_$VNAME2.bhv -f VNAME=$VNAME2     \
       START_POS=$START_POS2 LOITER_POS=$LOITER_POS2       


        
if [ ${JUST_MAKE} = "yes" ]; then
    echo "Files assembled; nothing launched; exiting per request."
    exit 0
fi

#-----------------------------------------------------------
#  Part 4: Launch the processes
#-----------------------------------------------------------
echo "Launching $VNAME1 MOOS Community. WARP is" $TIME_WARP
pAntler targ_$VNAME1.moos >& /dev/null &
echo "Launching $VNAME2 MOOS Community. WARP is" $TIME_WARP
pAntler targ_$VNAME2.moos >& /dev/null &
echo "Done"

#-----------------------------------------------------------
#  Part 5: Launch uMAC and kill everything upon exiting uMAC
#-----------------------------------------------------------
uMAC targ_$VNAME1.moos
uMAC targ_$VNAME2.moos

kill -- -$$