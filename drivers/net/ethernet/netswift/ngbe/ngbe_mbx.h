/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2019 - 2022 Beijing WangXun Technology Co., Ltd. */

#ifndef _NGBE_MBX_H_
#define _NGBE_MBX_H_

#define NGBE_VXMAILBOX_SIZE    (16)

/**
 * VF Registers
 **/
#define NGBE_VXMAILBOX         0x00600
#define NGBE_VXMAILBOX_REQ     ((0x1) << 0) /* Request for PF Ready bit */
#define NGBE_VXMAILBOX_ACK     ((0x1) << 1) /* Ack PF message received */
#define NGBE_VXMAILBOX_VFU     ((0x1) << 2) /* VF owns the mailbox buffer */
#define NGBE_VXMAILBOX_PFU     ((0x1) << 3) /* PF owns the mailbox buffer */
#define NGBE_VXMAILBOX_PFSTS   ((0x1) << 4) /* PF wrote a message in the MB */
#define NGBE_VXMAILBOX_PFACK   ((0x1) << 5) /* PF ack the previous VF msg */
#define NGBE_VXMAILBOX_RSTI    ((0x1) << 6) /* PF has reset indication */
#define NGBE_VXMAILBOX_RSTD    ((0x1) << 7) /* PF has indicated reset done */
#define NGBE_VXMAILBOX_R2C_BITS (NGBE_VXMAILBOX_RSTD | \
	    NGBE_VXMAILBOX_PFSTS | NGBE_VXMAILBOX_PFACK)

#define NGBE_VXMBMEM           0x00C00 /* 16*4B */

/**
 * PF Registers
 **/
#define NGBE_PXMAILBOX(i)      (0x00600 + (4 * (i))) /* i=[0,7] */
#define NGBE_PXMAILBOX_STS     ((0x1) << 0) /* Initiate message send to VF */
#define NGBE_PXMAILBOX_ACK     ((0x1) << 1) /* Ack message recv'd from VF */
#define NGBE_PXMAILBOX_VFU     ((0x1) << 2) /* VF owns the mailbox buffer */
#define NGBE_PXMAILBOX_PFU     ((0x1) << 3) /* PF owns the mailbox buffer */
#define NGBE_PXMAILBOX_RVFU    ((0x1) << 4) /* Reset VFU - used when VF stuck*/

#define NGBE_PXMBMEM(i)        (0x5000 + (64 * (i))) /* i=[0,7] */

#define NGBE_VFLRP(i)          (0x00490 + (4 * (i))) /* i=[0,1] */
#define NGBE_VFLRE          0x004A0
#define NGBE_VFLREC         0x004A8

/* SR-IOV specific macros */
#define NGBE_MBVFICR             0x00480
#define NGBE_MBVFICR_INDEX(vf) ((vf) >> 4)
#define NGBE_MBVFICR_VFREQ_MASK (0x0000FFFF) /* bits for VF messages */
#define NGBE_MBVFICR_VFREQ_VF1  (0x00000001) /* bit for VF 1 message */
#define NGBE_MBVFICR_VFACK_MASK (0xFFFF0000) /* bits for VF acks */
#define NGBE_MBVFICR_VFACK_VF1  (0x00010000) /* bit for VF 1 ack */

/* If it's a NGBE_VF_* msg then it originates in the VF and is sent to the
 * PF.  The reverse is true if it is NGBE_PF_*.
 * Message ACK's are the value or'd with 0xF0000000
 */
#define NGBE_VT_MSGTYPE_ACK    0x80000000
#define NGBE_VT_MSGTYPE_NACK   0x40000000
#define NGBE_VT_MSGTYPE_CTS    0x20000000
#define NGBE_VT_MSGINFO_SHIFT  16
/* bits 23:16 are used for extra info for certain messages */
#define NGBE_VT_MSGINFO_MASK   (0xFF << NGBE_VT_MSGINFO_SHIFT)

/* each element denotes a version of the API; existing numbers may not
 * change; any additions must go at the end
 */
enum ngbe_pfvf_api_rev {
	ngbe_mbox_api_null,
	ngbe_mbox_api_10,      /* API version 1.0, linux/freebsd VF driver */
	ngbe_mbox_api_11,      /* API version 1.1, linux/freebsd VF driver */
	ngbe_mbox_api_12,      /* API version 1.2, linux/freebsd VF driver */
	ngbe_mbox_api_13,	/* API version 1.3, linux/freebsd VF driver */
	ngbe_mbox_api_20,      /* API version 2.0, solaris Phase1 VF driver */
	ngbe_mbox_api_unknown, /* indicates that API version is not known */
};

/* mailbox API, legacy requests */
#define NGBE_VF_RESET          0x01 /* VF requests reset */
#define NGBE_VF_SET_MAC_ADDR   0x02 /* VF requests PF to set MAC addr */
#define NGBE_VF_SET_MULTICAST  0x03 /* VF requests PF to set MC addr */
#define NGBE_VF_SET_VLAN       0x04 /* VF requests PF to set VLAN */

/* mailbox API, version 1.0 VF requests */
#define NGBE_VF_SET_LPE        0x05 /* VF requests PF to set VMOLR.LPE */
#define NGBE_VF_SET_MACVLAN    0x06 /* VF requests PF for unicast filter */
#define NGBE_VF_API_NEGOTIATE  0x08 /* negotiate API version */

/* mailbox API, version 1.1 VF requests */
#define NGBE_VF_GET_QUEUES     0x09 /* get queue configuration */

/* mailbox API, version 1.2 VF requests */
#define NGBE_VF_GET_RETA      0x0a    /* VF request for RETA */
#define NGBE_VF_GET_RSS_KEY	0x0b    /* get RSS key */
#define NGBE_VF_UPDATE_XCAST_MODE	0x0c
#define NGBE_VF_BACKUP		0x8001 /* VF requests backup */

#define NGBE_VF_GET_LINK_STATUS	0x20 /* VF get link status from PF */

/* mode choices for IXGBE_VF_UPDATE_XCAST_MODE */
enum ngbevf_xcast_modes {
	NGBEVF_XCAST_MODE_NONE = 0,
	NGBEVF_XCAST_MODE_MULTI,
	NGBEVF_XCAST_MODE_ALLMULTI,
	NGBEVF_XCAST_MODE_PROMISC,
};

/* GET_QUEUES return data indices within the mailbox */
#define NGBE_VF_TX_QUEUES      1       /* number of Tx queues supported */
#define NGBE_VF_RX_QUEUES      2       /* number of Rx queues supported */
#define NGBE_VF_TRANS_VLAN     3       /* Indication of port vlan */
#define NGBE_VF_DEF_QUEUE      4       /* Default queue offset */

/* length of permanent address message returned from PF */
#define NGBE_VF_PERMADDR_MSG_LEN       4
/* word in permanent address message with the current multicast type */
#define NGBE_VF_MC_TYPE_WORD           3

#define NGBE_PF_CONTROL_MSG            0x0100 /* PF control message */

/* mailbox API, version 2.0 VF requests */
#define NGBE_VF_API_NEGOTIATE          0x08 /* negotiate API version */
#define NGBE_VF_GET_QUEUES             0x09 /* get queue configuration */
#define NGBE_VF_ENABLE_MACADDR         0x0A /* enable MAC address */
#define NGBE_VF_DISABLE_MACADDR        0x0B /* disable MAC address */
#define NGBE_VF_GET_MACADDRS           0x0C /* get all configured MAC addrs */
#define NGBE_VF_SET_MCAST_PROMISC      0x0D /* enable multicast promiscuous */
#define NGBE_VF_GET_MTU                0x0E /* get bounds on MTU */
#define NGBE_VF_SET_MTU                0x0F /* set a specific MTU */

/* mailbox API, version 2.0 PF requests */
#define NGBE_PF_TRANSPARENT_VLAN       0x0101 /* enable transparent vlan */

#define NGBE_VF_MBX_INIT_TIMEOUT       2000 /* number of retries on mailbox */
#define NGBE_VF_MBX_INIT_DELAY         500  /* microseconds between retries */

int ngbe_read_mbx(struct ngbe_hw *hw, u32 *msg, u16 size, u16 mbx_id);
int ngbe_write_mbx(struct ngbe_hw *hw, u32 *msg, u16 size, u16 mbx_id);
int ngbe_read_posted_mbx(struct ngbe_hw *hw, u32 *msg, u16 size, u16 mbx_id);
int ngbe_write_posted_mbx(struct ngbe_hw *hw, u32 *msg, u16 size, u16 mbx_id);
int ngbe_check_for_msg(struct ngbe_hw *hw, u16 mbx_id);
int ngbe_check_for_ack(struct ngbe_hw *hw, u16 mbx_id);
int ngbe_check_for_rst(struct ngbe_hw *hw, u16 mbx_id);
void ngbe_init_mbx_ops(struct ngbe_hw *hw);
void ngbe_init_mbx_params_vf(struct ngbe_hw *hw);
void ngbe_init_mbx_params_pf(struct ngbe_hw *hw);

#endif /* _NGBE_MBX_H_ */
