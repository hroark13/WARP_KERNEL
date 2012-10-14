/************************************************************************
*  版权所有 (C)2011,中兴通讯股份有限公司。
* 
* 文件名称： sim_card_detector.h
* 内容摘要： sim card检测驱动头文件
*
************************************************************************/
/*============================================================

                        EDIT HISTORY 

when              comment tag        who                   what, where, why                           
----------    ------------     -----------      --------------------------      
2011/06/28    yuanbo0010         yuanbo              add for sim card detecting
============================================================*/
#ifndef __SIM_CARD_DETECTOR_H
#define __SIM_CARD_DETECTOR_H

struct sim_card_detector_platform_data {
    unsigned int gpio;
#ifdef CONFIG_SIM_CARD_DETECTOR_IRQ_ZTE
    unsigned int irq;
    unsigned int irq_flags;
#endif
};

#endif /* __SIM_CARD_DETECTOR_H */
