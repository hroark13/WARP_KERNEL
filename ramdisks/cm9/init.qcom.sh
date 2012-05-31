#!/system/bin/sh
# Copyright (c) 2009-2010, Code Aurora Forum. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of Code Aurora nor
#       the names of its contributors may be used to endorse or promote
#       products derived from this software without specific prior written
#       permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

# /*<DTS2010092003634 jiazhifeng 20100925 begin*/
echo "ondemand" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor

# /*DTS2011081702455 zhangshufeng 2011-8-17 begin >*/
echo 245760 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq
# /*DTS2011081702455 zhangshufeng 2011-8-17 end >*/

# /*< DTS2011102601746 pengyu 20111026 begin */
# Set up_threshold initialize value from 95 to 80 to enhance performance
echo 95 > /sys/devices/system/cpu/cpu0/cpufreq/ondemand/up_threshold

# /* DTS2011102601746 pengyu 20111026 end >*/
# /*DTS2010092003634 jiazhifeng 20100925 end >*/
#
# start ril-daemon only for targets on which radio is present
#
baseband=`getprop ro.baseband`
netmgr=`getprop ro.use_data_netmgrd`

case "$baseband" in
    "msm" | "csfb" | "svlte2a" | "unknown")
    start ril-daemon
    start qmuxd
    case "$netmgr" in
        "true" | "True" | "TRUE")
        start netmgrd
    esac
esac

#
# Allow unique persistent serial numbers for devices connected via usb
# User needs to set unique usb serial number to persist.usb.serialno
#
serialno=`getprop persist.usb.serialno`
case "$serialno" in
    "") ;; #Do nothing here
    * )
    mount -t debugfs none /sys/kernel/debug
    echo "$serialno" > /sys/kernel/debug/android/serial_number
esac

#
# Allow persistent usb charging disabling
# User needs to set usb charging disabled in persist.usb.chgdisabled
#
target=`getprop ro.product.device`
usbchgdisabled=`getprop persist.usb.chgdisabled`
case "$usbchgdisabled" in
    "") ;; #Do nothing here
    * )
    case $target in
        "msm8660_surf" | "msm8660_csfb")
        echo "$usbchgdisabled" > /sys/module/pmic8058_charger/parameters/disabled
    esac
esac

# /*< DTS2010102301151 liyuping liliang  20101122 begin */
# enable hwvefs daemon process.
/system/bin/hwvefs /data/hwvefs -o allow_other &
# /* DTS2010102301151 liyuping liliang  20101122 end > */

/* < DTS2011031705399 renxigang 20110317 begin */
/system/bin/write_NV_114
/* DTS2011031705399 renxigang 20110317 end > */
case "$target" in
# DTS2011090801552 zhangcanyan 20110908 begin Add support for "ro.product.device=hwu8800Pro". 
# DTS2011080503149 zhangqijia 20110808 begin Add support for "ro.product.device=hwm886". 
# /*< DTS2011061005484 fangxinyong  20110613 begin */
# /* add support to u8860/c8860 */
# /*< DTS2011092001051 zhaosongwei  20110920 begin */
# /* add support to u8680/u8730 */
    "hwu8800Pro" | "hwu8680" | "hwu8730" | "hwm886" | "hwu8860" | "hwc8860" | "msm7630_surf" | "msm7630_1x" | "msm7630_fusion")
# /* DTS2011092001051 zhaosongwei  20110920 end > */
# /* DTS2011061005484 fangxinyong  20110613 end > */
# DTS2011080503149 zhangqijia 20110808 end
# DTS2011090801552 zhangcanyan 20110908 end 
        insmod /system/lib/modules/ss_mfcinit.ko
        insmod /system/lib/modules/ss_vencoder.ko
        insmod /system/lib/modules/ss_vdecoder.ko
        chmod 0666 /dev/ss_mfc_reg
        chmod 0666 /dev/ss_vdec
        chmod 0666 /dev/ss_venc

        case "$target" in
        "msm7630_fusion")
        start gpsone_daemon
        esac

        value=`cat /sys/devices/system/soc/soc0/hw_platform`

        case "$value" in
            "FFA" | "SVLTE_FFA")
             # linking to surf_keypad_qwerty.kcm.bin instead of surf_keypad_numeric.kcm.bin so that
             # the UI keyboard works fine.
             ln -s  /system/usr/keychars/surf_keypad_qwerty.kcm.bin /system/usr/keychars/surf_keypad.kcm.bin;;
            "Fluid")
             setprop ro.sf.lcd_density 240
             setprop qcom.bt.dev_power_class 2
             /system/bin/profiler_daemon&;;
            *)
             setprop ro.sf.lcd_density 240
             ln -s  /system/usr/keychars/surf_keypad_qwerty.kcm.bin /system/usr/keychars/surf_keypad.kcm.bin;;

        esac

# Dynamic Memory Managment (DMM) provides a sys file system to the userspace
# that can be used to plug in/out memory that has been configured as unstable.
# This unstable memory can be in Active or In-Active State.
# Each of which the userspace can request by writing to a sys file.

# ro.dev.dmm = 1; Indicates that DMM is enabled in the Android User Space. This
# property is set in the Android system properties file.

# ro.dev.dmm.dpd.start_address is set when the target has a 2x256Mb memory
# configuration. This is also used to indicate that the target is capable of
# setting EBI-1 to Deep Power Down or Self Refresh.

        mem="/sys/devices/system/memory"
        op=`cat $mem/movable_start_bytes`
        case "$op" in
           "0" )
                log -p i -t DMM DMM Disabled. movable_start_bytes not set: $op
            ;;

            "$mem/movable_start_bytes: No such file or directory " )
                log -p i -t DMM DMM Disabled. movable_start_bytes does not exist: $op
            ;;

            * )
                log -p i -t DMM DMM available. movable_start_bytes at $op
                movable_start_bytes=0x`cat $mem/movable_start_bytes`
                block_size_bytes=0x`cat $mem/block_size_bytes`
                block=$(($movable_start_bytes/$block_size_bytes))

                echo $movable_start_bytes > $mem/probe
                case "$?" in
                    "0" )
                        log -p i -t DMM $movable_start_bytes to physical hotplug succeeded.
                    ;;
                    * )
                        log -p e -t DMM $movable_start_bytes to physical hotplug failed.
                        return 1
                    ;;
                esac

               chown system system $mem/memory$block/state

                echo online > $mem/memory$block/state
                case "$?" in
                    "0" )
                        log -p i -t DMM \'echo online\' to logical hotplug succeeded.
                    ;;
                    * )
                        log -p e -t DMM \'echo online\' to logical hotplug failed.
                        return 1
                    ;;
                esac

                setprop ro.dev.dmm.dpd.start_address $movable_start_bytes
                setprop ro.dev.dmm.dpd.block $block
            ;;
        esac

        op=`cat $mem/low_power_memory_start_bytes`
        case "$op" in
            "0" )
                log -p i -t DMM Self-Refresh-Only Disabled. low_power_memory_start_bytes not set:$op
            ;;

            "$mem/low_power_memory_start_bytes No such file or directory " )
                log -p i -t DMM Self-Refresh-Only Disabled. low_power_memory_start_bytes does not exist:$op
            ;;

            * )
                log -p i -t DMM Self-Refresh-Only available. low_power_memory_start_bytes at $op
            ;;
        esac
        ;;
    "msm8660_surf")
        platformvalue=`cat /sys/devices/system/soc/soc0/hw_platform`
        case "$platformvalue" in
         "Fluid")
         setprop ro.sf.lcd_density 240;;
         esac

esac
