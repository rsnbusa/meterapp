Programm Flow

app_main: As usual entry point
    will determnine partially the program flow, speciality at the begining

-- Initial booting phase
    - flashsem created here since it controls access to Flash for all tasks and will immediately be used
    - NVS as per usual
    - read flash to get configurations and centinel guard. If no CENTINEL Erase/configure the Flash to defaults
    - Sanity checks for hardware
    - Init a bunch of variables, queues, etc. No tasks launched. See init vars routine
-- Main Task launched. This is the meat and bones of this Application.
    - Will get pulses from up to 8 meters and begin managing them, save to Fram etc
    - very simple rotuine, works on thersholds, for each Meter 1/10 of their BPK as upper limit
    --infinit loop
-- Kbd task launched if debug mode set in defines.h, extremely helpful to debug, work. Not necessary on final production but.....
    - It can lauch task, but ios not part of the main logic concern. Just a tool to test features and configurations
-- Now comes the Environment checks
    -- Is this node Mesh configured (has an ssid/pswd)? If not Provision it as per usual, BLE and AP selection. Can now be root
    -- This should be done ONCE and to a first Node so that it can help woith the "kids" installations
            -- THis way the
