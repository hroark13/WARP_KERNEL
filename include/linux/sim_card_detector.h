/************************************************************************
*  ��Ȩ���� (C)2011,����ͨѶ�ɷ����޹�˾��
* 
* �ļ����ƣ� sim_card_detector.h
* ����ժҪ�� sim card�������ͷ�ļ�
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
