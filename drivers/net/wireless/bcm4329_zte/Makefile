# bcm4329_zte

SRCBASE := $(shell /bin/pwd)/../..
DHDCFLAGS = -DDHD_GPL -DDHD_SCHED -DDHD_DEBUG -DSDTEST -DBDC -DTOE -DDHD_BCMEVENTS -DSHOW_EVENTS \
						-DSDIO_ISR_THREAD -DBCMSDIO -DDHD_GPL -DBCMLXSDMMC -DBCMPLATFORM_BUS -DCUSTOMER_HW \
						-DLINUX -DSRCBASE=\"$(SRCBASE)\" -DBCMDRIVER -DBCMDONGLEHOST -DDHDTHREAD -DBCMWPA2 -DBCMWAPI_WPI -DUNRELEASEDCHIP
						
						
DHDIFLAGS =	-Idrivers/net/wireless/bcm4329_zte -Idrivers/net/wireless/bcm4329_zte/include -Idrivers/net/wireless/bcm4329_zte/shared \
						-Idrivers/net/wireless/bcm4329_zte/sys -Idrivers/net/wireless/bcm4329_zte/dongle -Idrivers/net/wireless/bcm4329_zte/wl/sys

DHDOFILES = dhd_linux.o linux_osl.o bcmutils.o dhd_common.o dhd_custom_gpio.o \
	wl_iw.o siutils.o sbutils.o aiutils.o hndpmu.o bcmwifi.o dhd_sdio.o   \
	dhd_linux_sched.o dhd_cdc.o bcmsdh_sdmmc.o bcmsdh.o bcmsdh_linux.o    \
	bcmsdh_sdmmc_linux.o bcmutils.o

obj-$(CONFIG_BCM4329_ZTE) += dhd.o
dhd-objs += $(DHDOFILES)
EXTRA_CFLAGS = $(DHDCFLAGS) $(DHDIFLAGS)
EXTRA_LDFLAGS += --strip-debug
