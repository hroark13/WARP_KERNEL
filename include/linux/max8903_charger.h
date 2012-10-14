/******************************************************************************
  @file    max8903_charger.h
  @brief   Maxim 8903 USB/Adapter Charger Driver
  @author  liuzhongzhi 2011/05/20

  DESCRIPTION

  INITIALIZATION AND SEQUENCING REQUIREMENTS

  ---------------------------------------------------------------------------
  Copyright (c) 2011 ZTE Incorporated. All Rights Reserved. 
  ZTE Proprietary and Confidential.
  ---------------------------------------------------------------------------
******************************************************************************/
#ifndef __MAX8903_CHARGER_H__
#define __MAX8903_CHARGER_H__

struct max8903_platform_data {
	int	dock_det; /* DOCK_DET pin */
	int cen;	/* Charger Enable input */
	int chg;	/* Charger status output */
	int flt;	/* Fault output */
	int usus;	/* USB suspend */
	int irq;	/* DC(Adapter) Power OK output */
};

void max8903_chg_connected(enum chg_type chg_type);
#endif /* __MAX8903_CHARGER_H__ */
