
#!/bin/bash
#to batch FLASh several esp32 devices , who are set in download mode manually or automatically.
#if change espidf version, change the /Users/rsn/.espressif5.3/python_env/idf5.3_py3.11_env/bin/python  to curent one 
# parameters are the usb device general name,e x /dev/tty.usbserial, and then the ports, ex 0001 5 03, etc giving /dev/tty.usbserial-0001 etc
#lost of commented statements but dont delete them might be usefull here or somewhere else
#example of cmd 
#sh batchflash.sh /dev/tty.usbserial 0001 3

# from build command this message says how to flash nada just git test

# python -m esptool --chip esp32 -b 1500000 --before default_reset --after hard_reset  --port /dev/tty.usbserial-5 write_flash --flash_mode dio --flash_size 4MB --flash_freq 40m 0x1000 bootloader/bootloader.bin 0x20000 metermgr.bin 0x8000 partition_table/partition-table.bin 0x16000 ota_data_initial.bin 
# or from the "/Volumes/MacData/IoTProjects/MeterIoT/MeterIoTApp/build" directory
# python -m esptool --chip esp32 -b 1500000 --before default_reset --after hard_reset write_flash "@flash_args"

function msgBox() {
  osascript <<EOT
    tell app "System Events"
      display dialog "$@" buttons {"OK"} default button 1 with icon caution with title "$(basename $0)"
      ## icon 0=stop,icon 1=software,icon 2=caution
      return  -- Suppress result
    end tell
EOT
}
waitall()
{
    myArray=( "$@" )    #all the parameters
    son=${#myArray[*]}  # how many params
    nada=0
        # echo "Wait for $son parameters ${myArray[*]}"
    # for keys in "${!myArray[@]}"; do echo "$keys"; done
    while [ $son -gt 0 ] #we will substract for every Non Running ps call
    do
        for i in "${!myArray[@]}"
        do
        # echo "Key $i ${myArray[$i]}"
        # pidd=${myArray[$i]}
        # echo "Key of $pidd is ${!myArray[$pidd]}"
        # echo "Program Id $pidd"
        var="$(ps -p ${myArray[$i]} -o '%cpu'  | sed 1d) "
        # echo " Variable [$var] ${#var}"
            if [ ${#var} -gt 1 ]
            then
                # echo -n "."
                nada=0
                # echo "$ppid is running"
                # Do something knowing the pid exists, i.e. the process with $PID is running
            else
                # echo "Son $son"
                son=$(( son - 1))
                unset myArray[$i]
                # myArray=( ${myArray[@]/$$pidd} )
                # echo "new Array ${myArray[*]} remove $pidd"
            fi
        sleep 1
        done
done
# echo "Done all Flashing"

}

while [ $# -gt 0 ]; do

   if [[ $1 == *"--"* ]]; then
        v="${1/--/}"
        declare $v="$2"
   fi

  shift
done

# Two possible parameters, chip and erase, usage --chip <name> --erase <Y to erase or nothing>
if [ "$chip" == "" ];  then
    echo "No Chip specified using ESP32S3"
    chip="esp32s3"
fi

if [ "$chip" == "esp32s3" ]; then
    offset=0x0
else
    offset=0x1000
fi

#first see if the esp-idf-monitor is using the connection and kill all /dev/tty.u devices 
var="$(ps aux |grep /dev/tty.u |grep -v grep | grep esp_idf_monitor| awk '{print $2}')"
if [ ${#var} -gt 0 ]; then
    kill -9 $var 
fi
    #  kill -9 $(ps  u |grep /dev/tty.u |grep -v grep | grep esp_idf_monitor| awk '{print $2}') 
var="$(ps aux |grep "CoolTerm" |grep -v "grep"|awk '{print $2}')"
if [ ${#var} -gt 0 ]; then
    kill -9 $var 
fi
# kill -9 $(ps aux |grep "CoolTerm" |grep -v "grep"|awk '{print $2}') > /dev/null # coolterm in case its holding any ports
#for some reason ps cannot find certain VSCode terminals but lsof can
var="$(lsof /dev/tty* |grep "/dev/tty.u" |awk '{print $2}')"
if [ ${#var} -gt 0 ]; then
    kill -9 $var 
fi
# kill -9 $(lsof |grep "/dev/tty.u" |awk '{print $2}')
#locate python and esp latest version
donde=($(ls ~/.espressif/python_env | sort -rt'-' -k2,2rn | head -1 ))
version=($(ls ~/esp | sort -rt'-' -k2,2rn | head -1))
echo "Using Python ${donde} & Version ${version}"
#get a list of /dev/tty.u* devices
devs=($(ls /dev | grep tty.u))  #find project name by looking for .bin but exclude(grep -v) otaxxx.bin
cnt=${#devs[@]}     # how many devices found
# I CANNOT set it in FLASH mode so there it goes
if [ "$erase" == "Y" ]; then
echo "Erasing  $cnt devices ["${devs[*]}]""
else
echo "Flashing $cnt devices ["${devs[*]}]""
fi
declare -a lospids          #add to pids under work 
project=$(find ./build -maxdepth 1 -name '*.bin' | grep -v "ota" | awk '{print $1}')  #find project name by looking for .bin but exclude(grep -v) otaxxx.bin
num=10
projname=${project:8}   #get name without the .build/ start at positon 8
echo "Project $project"
# erase
if [ "$erase" == "Y" ]; then

for (( i=0;i<$cnt;i++)); do
    portt="${devs[$i]}"
cd build
lvar="/Users/rsn/.espressif/python_env/$donde/bin/python /Users/rsn/esp/master/esp-idf/components/esptool_py/esptool/esptool.py -p "/dev/$portt" -b 1500000 --before default_reset --after hard_reset --chip esp32s3 write_flash --flash_mode dio --flash_freq 80m --flash_size 4MB 0x0 bootloader/bootloader.bin 0x20000 metermgr.bin 0x8000 partition_table/partition-table.bin 0xf000 ota_data_initial.bin >/tmp/f$num 2>/dev/null &"
# /Users/rsn/.espressif/python_env/idf5.5_py3.9_env/bin/python /Users/rsn/esp/master/esp-idf/components/esptool_py/esptool/esptool.py -p /dev/tty.usbserial-0001 -b 1500000 --before default_reset --after hard_reset --chip esp32s3 write_flash --flash_mode dio --flash_freq 80m --flash_size 4MB 0x0 bootloader/bootloader.bin 0x20000 metermgr.bin 0x8000 partition_table/partition-table.bin 0xf000 ota_data_initial.bin 
# lvar="/Users/rsn/.espressif/python_env/$donde/bin/python /Users/rsn/esp/$version/esp-idf/components/esptool_py/esptool/esptool.py -p "/dev/$portt" -b 1500000 erase_flash  >/tmp/f$num 2>/dev/null &"
# echo "cmd  $lvar"
eval $lvar      #execute it
    lospids+="$! "  # need the space is just a bunch of letters now
    ((num++))       # for tmps files
done
waitall  $lospids
echo "Done erase. Set to Download again"
num=10
for (( i=0;i<$cnt;i++)); do
    portt="${devs[$i]}"

    err="$(cat /tmp/f$num | grep Leaving...) "
    if [ ${#err} -gt 10 ]; then
        mensaje="SUCCESS...Port $portt ended erasing $projname"
    else
        err="$(cat /tmp/f$num | grep busy)"
        if [ ${#err} -gt 0 ]; then
            mensaje="PORT BUSY...Port $portt already open"
        else
            mensaje="$(cat /tmp/f$num)"
        fi
    fi
    # err="$(rm /tmp/f$num)"
    ((num++))
    msgBox $mensaje
    msgBox "Set to download again"
done 
# exit 0
echo "Flashing Firmware"
fi
num=10
declare -a lospids          #add to pids under work 

for (( i=0;i<$cnt;i++)); do
    portt="${devs[$i]}"

lvar="/Users/rsn/.espressif/python_env/$donde/bin/python /Users/rsn/esp/$version/esp-idf/components/esptool_py/esptool/esptool.py -p "/dev/$portt" -b 1500000 --before default_reset --after hard_reset --chip $chip write_flash --flash_mode dio --flash_freq 80m --flash_size 4MB $offset  ./build/bootloader/bootloader.bin 0x10000 $project 0x8000 ./build/partition_table/partition-table.bin  >/tmp/f$num 2>/dev/null &"
# echo "cmd  $lvar"
eval $lvar      #execute it
    lospids+="$! "  # need the space is just a bunch of letters now
    ((num++))       # for tmps files
done
# echo "Pids ${lospids[@]} for $# ports"
waitall  $lospids
 num=10
for (( i=0;i<$cnt;i++)); do
    portt="${devs[$i]}"

    err="$(cat /tmp/f$num | grep Leaving...) "
    if [ ${#err} -gt 10 ]; then
        mensaje="SUCCESS...Port $portt ended flashing $projname"
    else
        err="$(cat /tmp/f$num | grep busy)"
        if [ ${#err} -gt 0 ]; then
            mensaje="PORT BUSY...Port $portt already open"
        else
            mensaje="$(cat /tmp/f$num)"
        fi
    fi
    # err="$(rm /tmp/f$num)"
    ((num++))
    msgBox $mensaje
done 
# kill $(ps u | grep Python | grep -v grep | awk '{print $2}')
kill -9 $(ps u |grep /dev/tty. |grep -v grep | awk '{print $2}')
