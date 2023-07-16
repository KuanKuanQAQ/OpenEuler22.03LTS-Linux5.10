/* SPDX-License-Identifier: GPL-2.0-only WITH Linux-syscall-note */
/*
 * include/uapi/linux/ethtool_netlink.h - netlink interface for ethtool
 *
 * See Documentation/networking/ethtool-netlink.rst in kernel source tree for
 * doucumentation of the interface.
 */

#ifndef _UAPI_LINUX_ETHTOOL_NETLINK_H_
#define _UAPI_LINUX_ETHTOOL_NETLINK_H_

#include <linux/ethtool.h>

/* message types - userspace to kernel */
enum {
	ETHTOOL_MSG_USER_NONE,
	ETHTOOL_MSG_STRSET_GET,
	ETHTOOL_MSG_LINKINFO_GET,
	ETHTOOL_MSG_LINKINFO_SET,
	ETHTOOL_MSG_LINKMODES_GET,
	ETHTOOL_MSG_LINKMODES_SET,
	ETHTOOL_MSG_LINKSTATE_GET,
	ETHTOOL_MSG_DEBUG_GET,
	ETHTOOL_MSG_DEBUG_SET,
	ETHTOOL_MSG_WOL_GET,
	ETHTOOL_MSG_WOL_SET,
	ETHTOOL_MSG_FEATURES_GET,
	ETHTOOL_MSG_FEATURES_SET,
	ETHTOOL_MSG_PRIVFLAGS_GET,
	ETHTOOL_MSG_PRIVFLAGS_SET,
	ETHTOOL_MSG_RINGS_GET,
	ETHTOOL_MSG_RINGS_SET,
	ETHTOOL_MSG_CHANNELS_GET,
	ETHTOOL_MSG_CHANNELS_SET,
	ETHTOOL_MSG_COALESCE_GET,
	ETHTOOL_MSG_COALESCE_SET,
	ETHTOOL_MSG_PAUSE_GET,
	ETHTOOL_MSG_PAUSE_SET,
	ETHTOOL_MSG_EEE_GET,
	ETHTOOL_MSG_EEE_SET,
	ETHTOOL_MSG_TSINFO_GET,
	ETHTOOL_MSG_CABLE_TEST_ACT,
	ETHTOOL_MSG_CABLE_TEST_TDR_ACT,
	ETHTOOL_MSG_TUNNEL_INFO_GET,

	/* add new constants above here */
	__ETHTOOL_MSG_USER_CNT,
	ETHTOOL_MSG_USER_MAX = __ETHTOOL_MSG_USER_CNT - 1
};

/* message types - kernel to userspace */
enum {
	ETHTOOL_MSG_KERNEL_NONE,
	ETHTOOL_MSG_STRSET_GET_REPLY,
	ETHTOOL_MSG_LINKINFO_GET_REPLY,
	ETHTOOL_MSG_LINKINFO_NTF,
	ETHTOOL_MSG_LINKMODES_GET_REPLY,
	ETHTOOL_MSG_LINKMODES_NTF,
	ETHTOOL_MSG_LINKSTATE_GET_REPLY,
	ETHTOOL_MSG_DEBUG_GET_REPLY,
	ETHTOOL_MSG_DEBUG_NTF,
	ETHTOOL_MSG_WOL_GET_REPLY,
	ETHTOOL_MSG_WOL_NTF,
	ETHTOOL_MSG_FEATURES_GET_REPLY,
	ETHTOOL_MSG_FEATURES_SET_REPLY,
	ETHTOOL_MSG_FEATURES_NTF,
	ETHTOOL_MSG_PRIVFLAGS_GET_REPLY,
	ETHTOOL_MSG_PRIVFLAGS_NTF,
	ETHTOOL_MSG_RINGS_GET_REPLY,
	ETHTOOL_MSG_RINGS_NTF,
	ETHTOOL_MSG_CHANNELS_GET_REPLY,
	ETHTOOL_MSG_CHANNELS_NTF,
	ETHTOOL_MSG_COALESCE_GET_REPLY,
	ETHTOOL_MSG_COALESCE_NTF,
	ETHTOOL_MSG_PAUSE_GET_REPLY,
	ETHTOOL_MSG_PAUSE_NTF,
	ETHTOOL_MSG_EEE_GET_REPLY,
	ETHTOOL_MSG_EEE_NTF,
	ETHTOOL_MSG_TSINFO_GET_REPLY,
	ETHTOOL_MSG_CABLE_TEST_NTF,
	ETHTOOL_MSG_CABLE_TEST_TDR_NTF,
	ETHTOOL_MSG_TUNNEL_INFO_GET_REPLY,

	/* add new constants above here */
	__ETHTOOL_MSG_KERNEL_CNT,
	ETHTOOL_MSG_KERNEL_MAX = __ETHTOOL_MSG_KERNEL_CNT - 1
};

/* request header */

/* use compact bitsets in reply */
#define ETHTOOL_FLAG_COMPACT_BITSETS	(1 << 0)
/* provide optional reply for SET or ACT requests */
#define ETHTOOL_FLAG_OMIT_REPLY	(1 << 1)
/* request statistics, if supported by the driver */
#define ETHTOOL_FLAG_STATS		(1 << 2)

#define ETHTOOL_FLAG_ALL (ETHTOOL_FLAG_COMPACT_BITSETS | \
			  ETHTOOL_FLAG_OMIT_REPLY | \
			  ETHTOOL_FLAG_STATS)

enum {
	ETHTOOL_A_HEADER_UNSPEC,
	ETHTOOL_A_HEADER_DEV_INDEX,		/* u32 */
	ETHTOOL_A_HEADER_DEV_NAME,		/* string */
	ETHTOOL_A_HEADER_FLAGS,			/* u32 - ETHTOOL_FLAG_* */

	/* add new constants above here */
	__ETHTOOL_A_HEADER_CNT,
	ETHTOOL_A_HEADER_MAX = __ETHTOOL_A_HEADER_CNT - 1
};

/* bit sets */

enum {
	ETHTOOL_A_BITSET_BIT_UNSPEC,
	ETHTOOL_A_BITSET_BIT_INDEX,		/* u32 */
	ETHTOOL_A_BITSET_BIT_NAME,		/* string */
	ETHTOOL_A_BITSET_BIT_VALUE,		/* flag */

	/* add new constants above here */
	__ETHTOOL_A_BITSET_BIT_CNT,
	ETHTOOL_A_BITSET_BIT_MAX = __ETHTOOL_A_BITSET_BIT_CNT - 1
};

enum {
	ETHTOOL_A_BITSET_BITS_UNSPEC,
	ETHTOOL_A_BITSET_BITS_BIT,		/* nest - _A_BITSET_BIT_* */

	/* add new constants above here */
	__ETHTOOL_A_BITSET_BITS_CNT,
	ETHTOOL_A_BITSET_BITS_MAX = __ETHTOOL_A_BITSET_BITS_CNT - 1
};

enum {
	ETHTOOL_A_BITSET_UNSPEC,
	ETHTOOL_A_BITSET_NOMASK,		/* flag */
	ETHTOOL_A_BITSET_SIZE,			/* u32 */
	ETHTOOL_A_BITSET_BITS,			/* nest - _A_BITSET_BITS_* */
	ETHTOOL_A_BITSET_VALUE,			/* binary */
	ETHTOOL_A_BITSET_MASK,			/* binary */

	/* add new constants above here */
	__ETHTOOL_A_BITSET_CNT,
	ETHTOOL_A_BITSET_MAX = __ETHTOOL_A_BITSET_CNT - 1
};

/* string sets */

enum {
	ETHTOOL_A_STRING_UNSPEC,
	ETHTOOL_A_STRING_INDEX,			/* u32 */
	ETHTOOL_A_STRING_VALUE,			/* string */

	/* add new constants above here */
	__ETHTOOL_A_STRING_CNT,
	ETHTOOL_A_STRING_MAX = __ETHTOOL_A_STRING_CNT - 1
};

enum {
	ETHTOOL_A_STRINGS_UNSPEC,
	ETHTOOL_A_STRINGS_STRING,		/* nest - _A_STRINGS_* */

	/* add new constants above here */
	__ETHTOOL_A_STRINGS_CNT,
	ETHTOOL_A_STRINGS_MAX = __ETHTOOL_A_STRINGS_CNT - 1
};

enum {
	ETHTOOL_A_STRINGSET_UNSPEC,
	ETHTOOL_A_STRINGSET_ID,			/* u32 */
	ETHTOOL_A_STRINGSET_COUNT,		/* u32 */
	ETHTOOL_A_STRINGSET_STRINGS,		/* nest - _A_STRINGS_* */

	/* add new constants above here */
	__ETHTOOL_A_STRINGSET_CNT,
	ETHTOOL_A_STRINGSET_MAX = __ETHTOOL_A_STRINGSET_CNT - 1
};

enum {
	ETHTOOL_A_STRINGSETS_UNSPEC,
	ETHTOOL_A_STRINGSETS_STRINGSET,		/* nest - _A_STRINGSET_* */

	/* add new constants above here */
	__ETHTOOL_A_STRINGSETS_CNT,
	ETHTOOL_A_STRINGSETS_MAX = __ETHTOOL_A_STRINGSETS_CNT - 1
};

/* STRSET */

enum {
	ETHTOOL_A_STRSET_UNSPEC,
	ETHTOOL_A_STRSET_HEADER,		/* nest - _A_HEADER_* */
	ETHTOOL_A_STRSET_STRINGSETS,		/* nest - _A_STRINGSETS_* */
	ETHTOOL_A_STRSET_COUNTS_ONLY,		/* flag */

	/* add new constants above here */
	__ETHTOOL_A_STRSET_CNT,
	ETHTOOL_A_STRSET_MAX = __ETHTOOL_A_STRSET_CNT - 1
};

/* LINKINFO */

enum {
	ETHTOOL_A_LINKINFO_UNSPEC,
	ETHTOOL_A_LINKINFO_HEADER,		/* nest - _A_HEADER_* */
	ETHTOOL_A_LINKINFO_PORT,		/* u8 */
	ETHTOOL_A_LINKINFO_PHYADDR,		/* u8 */
	ETHTOOL_A_LINKINFO_TP_MDIX,		/* u8 */
	ETHTOOL_A_LINKINFO_TP_MDIX_CTRL,	/* u8 */
	ETHTOOL_A_LINKINFO_TRANSCEIVER,		/* u8 */

	/* add new constants above here */
	__ETHTOOL_A_LINKINFO_CNT,
	ETHTOOL_A_LINKINFO_MAX = __ETHTOOL_A_LINKINFO_CNT - 1
};

/* LINKMODES */

enum {
	ETHTOOL_A_LINKMODES_UNSPEC,
	ETHTOOL_A_LINKMODES_HEADER,		/* nest - _A_HEADER_* */
	ETHTOOL_A_LINKMODES_AUTONEG,		/* u8 */
	ETHTOOL_A_LINKMODES_OURS,		/* bitset */
	ETHTOOL_A_LINKMODES_PEER,		/* bitset */
	ETHTOOL_A_LINKMODES_SPEED,		/* u32 */
	ETHTOOL_A_LINKMODES_DUPLEX,		/* u8 */
	ETHTOOL_A_LINKMODES_MASTER_SLAVE_CFG,	/* u8 */
	ETHTOOL_A_LINKMODES_MASTER_SLAVE_STATE,	/* u8 */

	/* add new constants above here */
	__ETHTOOL_A_LINKMODES_CNT,
	ETHTOOL_A_LINKMODES_MAX = __ETHTOOL_A_LINKMODES_CNT - 1
};

/* LINKSTATE */

enum {
	ETHTOOL_A_LINKSTATE_UNSPEC,
	ETHTOOL_A_LINKSTATE_HEADER,		/* nest - _A_HEADER_* */
	ETHTOOL_A_LINKSTATE_LINK,		/* u8 */
	ETHTOOL_A_LINKSTATE_SQI,		/* u32 */
	ETHTOOL_A_LINKSTATE_SQI_MAX,		/* u32 */
	ETHTOOL_A_LINKSTATE_EXT_STATE,		/* u8 */
	ETHTOOL_A_LINKSTATE_EXT_SUBSTATE,	/* u8 */

	/* add new constants above here */
	__ETHTOOL_A_LINKSTATE_CNT,
	ETHTOOL_A_LINKSTATE_MAX = __ETHTOOL_A_LINKSTATE_CNT - 1
};

/* DEBUG */

enum {
	ETHTOOL_A_DEBUG_UNSPEC,
	ETHTOOL_A_DEBUG_HEADER,			/* nest - _A_HEADER_* */
	ETHTOOL_A_DEBUG_MSGMASK,		/* bitset */

	/* add new constants above here */
	__ETHTOOL_A_DEBUG_CNT,
	ETHTOOL_A_DEBUG_MAX = __ETHTOOL_A_DEBUG_CNT - 1
};

/* WOL */

enum {
	ETHTOOL_A_WOL_UNSPEC,
	ETHTOOL_A_WOL_HEADER,			/* nest - _A_HEADER_* */
	ETHTOOL_A_WOL_MODES,			/* bitset */
	ETHTOOL_A_WOL_SOPASS,			/* binary */

	/* add new constants above here */
	__ETHTOOL_A_WOL_CNT,
	ETHTOOL_A_WOL_MAX = __ETHTOOL_A_WOL_CNT - 1
};

/* FEATURES */

enum {
	ETHTOOL_A_FEATURES_UNSPEC,
	ETHTOOL_A_FEATURES_HEADER,			/* nest - _A_HEADER_* */
	ETHTOOL_A_FEATURES_HW,				/* bitset */
	ETHTOOL_A_FEATURES_WANTED,			/* bitset */
	ETHTOOL_A_FEATURES_ACTIVE,			/* bitset */
	ETHTOOL_A_FEATURES_NOCHANGE,			/* bitset */

	/* add new constants above here */
	__ETHTOOL_A_FEATURES_CNT,
	ETHTOOL_A_FEATURES_MAX = __ETHTOOL_A_FEATURES_CNT - 1
};

/* PRIVFLAGS */

enum {
	ETHTOOL_A_PRIVFLAGS_UNSPEC,
	ETHTOOL_A_PRIVFLAGS_HEADER,			/* nest - _A_HEADER_* */
	ETHTOOL_A_PRIVFLAGS_FLAGS,			/* bitset */

	/* add new constants above here */
	__ETHTOOL_A_PRIVFLAGS_CNT,
	ETHTOOL_A_PRIVFLAGS_MAX = __ETHTOOL_A_PRIVFLAGS_CNT - 1
};

/* RINGS */

enum {
	ETHTOOL_A_RINGS_UNSPEC,
	ETHTOOL_A_RINGS_HEADER,				/* nest - _A_HEADER_* */
	ETHTOOL_A_RINGS_RX_MAX,				/* u32 */
	ETHTOOL_A_RINGS_RX_MINI_MAX,			/* u32 */
	ETHTOOL_A_RINGS_RX_JUMBO_MAX,			/* u32 */
	ETHTOOL_A_RINGS_TX_MAX,				/* u32 */
	ETHTOOL_A_RINGS_RX,				/* u32 */
	ETHTOOL_A_RINGS_RX_MINI,			/* u32 */
	ETHTOOL_A_RINGS_RX_JUMBO,			/* u32 */
	ETHTOOL_A_RINGS_TX,				/* u32 */
	ETHTOOL_A_RINGS_RX_BUF_LEN,                     /* u32 */

	/* add new constants above here */
	__ETHTOOL_A_RINGS_CNT,
	ETHTOOL_A_RINGS_MAX = (__ETHTOOL_A_RINGS_CNT - 1)
};

/* CHANNELS */

enum {
	ETHTOOL_A_CHANNELS_UNSPEC,
	ETHTOOL_A_CHANNELS_HEADER,			/* nest - _A_HEADER_* */
	ETHTOOL_A_CHANNELS_RX_MAX,			/* u32 */
	ETHTOOL_A_CHANNELS_TX_MAX,			/* u32 */
	ETHTOOL_A_CHANNELS_OTHER_MAX,			/* u32 */
	ETHTOOL_A_CHANNELS_COMBINED_MAX,		/* u32 */
	ETHTOOL_A_CHANNELS_RX_COUNT,			/* u32 */
	ETHTOOL_A_CHANNELS_TX_COUNT,			/* u32 */
	ETHTOOL_A_CHANNELS_OTHER_COUNT,			/* u32 */
	ETHTOOL_A_CHANNELS_COMBINED_COUNT,		/* u32 */

	/* add new constants above here */
	__ETHTOOL_A_CHANNELS_CNT,
	ETHTOOL_A_CHANNELS_MAX = (__ETHTOOL_A_CHANNELS_CNT - 1)
};

/* COALESCE */

enum {
	ETHTOOL_A_COALESCE_UNSPEC,
	ETHTOOL_A_COALESCE_HEADER,			/* nest - _A_HEADER_* */
	ETHTOOL_A_COALESCE_RX_USECS,			/* u32 */
	ETHTOOL_A_COALESCE_RX_MAX_FRAMES,		/* u32 */
	ETHTOOL_A_COALESCE_RX_USECS_IRQ,		/* u32 */
	ETHTOOL_A_COALESCE_RX_MAX_FRAMES_IRQ,		/* u32 */
	ETHTOOL_A_COALESCE_TX_USECS,			/* u32 */
	ETHTOOL_A_COALESCE_TX_MAX_FRAMES,		/* u32 */
	ETHTOOL_A_COALESCE_TX_USECS_IRQ,		/* u32 */
	ETHTOOL_A_COALESCE_TX_MAX_FRAMES_IRQ,		/* u32 */
	ETHTOOL_A_COALESCE_STATS_BLOCK_USECS,		/* u32 */
	ETHTOOL_A_COALESCE_USE_ADAPTIVE_RX,		/* u8 */
	ETHTOOL_A_COALESCE_USE_ADAPTIVE_TX,		/* u8 */
	ETHTOOL_A_COALESCE_PKT_RATE_LOW,		/* u32 */
	ETHTOOL_A_COALESCE_RX_USECS_LOW,		/* u32 */
	ETHTOOL_A_COALESCE_RX_MAX_FRAMES_LOW,		/* u32 */
	ETHTOOL_A_COALESCE_TX_USECS_LOW,		/* u32 */
	ETHTOOL_A_COALESCE_TX_MAX_FRAMES_LOW,		/* u32 */
	ETHTOOL_A_COALESCE_PKT_RATE_HIGH,		/* u32 */
	ETHTOOL_A_COALESCE_RX_USECS_HIGH,		/* u32 */
	ETHTOOL_A_COALESCE_RX_MAX_FRAMES_HIGH,		/* u32 */
	ETHTOOL_A_COALESCE_TX_USECS_HIGH,		/* u32 */
	ETHTOOL_A_COALESCE_TX_MAX_FRAMES_HIGH,		/* u32 */
	ETHTOOL_A_COALESCE_RATE_SAMPLE_INTERVAL,	/* u32 */
	ETHTOOL_A_COALESCE_USE_CQE_MODE_TX,		/* u8 */
	ETHTOOL_A_COALESCE_USE_CQE_MODE_RX,		/* u8 */

	/* add new constants above here */
	__ETHTOOL_A_COALESCE_CNT,
	ETHTOOL_A_COALESCE_MAX = (__ETHTOOL_A_COALESCE_CNT - 1)
};

/* PAUSE */

enum {
	ETHTOOL_A_PAUSE_UNSPEC,
	ETHTOOL_A_PAUSE_HEADER,				/* nest - _A_HEADER_* */
	ETHTOOL_A_PAUSE_AUTONEG,			/* u8 */
	ETHTOOL_A_PAUSE_RX,				/* u8 */
	ETHTOOL_A_PAUSE_TX,				/* u8 */
	ETHTOOL_A_PAUSE_STATS,				/* nest - _PAUSE_STAT_* */

	/* add new constants above here */
	__ETHTOOL_A_PAUSE_CNT,
	ETHTOOL_A_PAUSE_MAX = (__ETHTOOL_A_PAUSE_CNT - 1)
};

enum {
	ETHTOOL_A_PAUSE_STAT_UNSPEC,
	ETHTOOL_A_PAUSE_STAT_PAD,

	ETHTOOL_A_PAUSE_STAT_TX_FRAMES,
	ETHTOOL_A_PAUSE_STAT_RX_FRAMES,

	/* add new constants above here
	 * adjust ETHTOOL_PAUSE_STAT_CNT if adding non-stats!
	 */
	__ETHTOOL_A_PAUSE_STAT_CNT,
	ETHTOOL_A_PAUSE_STAT_MAX = (__ETHTOOL_A_PAUSE_STAT_CNT - 1)
};

/* EEE */

enum {
	ETHTOOL_A_EEE_UNSPEC,
	ETHTOOL_A_EEE_HEADER,				/* nest - _A_HEADER_* */
	ETHTOOL_A_EEE_MODES_OURS,			/* bitset */
	ETHTOOL_A_EEE_MODES_PEER,			/* bitset */
	ETHTOOL_A_EEE_ACTIVE,				/* u8 */
	ETHTOOL_A_EEE_ENABLED,				/* u8 */
	ETHTOOL_A_EEE_TX_LPI_ENABLED,			/* u8 */
	ETHTOOL_A_EEE_TX_LPI_TIMER,			/* u32 */

	/* add new constants above here */
	__ETHTOOL_A_EEE_CNT,
	ETHTOOL_A_EEE_MAX = (__ETHTOOL_A_EEE_CNT - 1)
};

/* TSINFO */

enum {
	ETHTOOL_A_TSINFO_UNSPEC,
	ETHTOOL_A_TSINFO_HEADER,			/* nest - _A_HEADER_* */
	ETHTOOL_A_TSINFO_TIMESTAMPING,			/* bitset */
	ETHTOOL_A_TSINFO_TX_TYPES,			/* bitset */
	ETHTOOL_A_TSINFO_RX_FILTERS,			/* bitset */
	ETHTOOL_A_TSINFO_PHC_INDEX,			/* u32 */

	/* add new constants above here */
	__ETHTOOL_A_TSINFO_CNT,
	ETHTOOL_A_TSINFO_MAX = (__ETHTOOL_A_TSINFO_CNT - 1)
};

/* CABLE TEST */

enum {
	ETHTOOL_A_CABLE_TEST_UNSPEC,
	ETHTOOL_A_CABLE_TEST_HEADER,		/* nest - _A_HEADER_* */

	/* add new constants above here */
	__ETHTOOL_A_CABLE_TEST_CNT,
	ETHTOOL_A_CABLE_TEST_MAX = __ETHTOOL_A_CABLE_TEST_CNT - 1
};

/* CABLE TEST NOTIFY */
enum {
	ETHTOOL_A_CABLE_RESULT_CODE_UNSPEC,
	ETHTOOL_A_CABLE_RESULT_CODE_OK,
	ETHTOOL_A_CABLE_RESULT_CODE_OPEN,
	ETHTOOL_A_CABLE_RESULT_CODE_SAME_SHORT,
	ETHTOOL_A_CABLE_RESULT_CODE_CROSS_SHORT,
};

enum {
	ETHTOOL_A_CABLE_PAIR_A,
	ETHTOOL_A_CABLE_PAIR_B,
	ETHTOOL_A_CABLE_PAIR_C,
	ETHTOOL_A_CABLE_PAIR_D,
};

enum {
	ETHTOOL_A_CABLE_RESULT_UNSPEC,
	ETHTOOL_A_CABLE_RESULT_PAIR,		/* u8 ETHTOOL_A_CABLE_PAIR_ */
	ETHTOOL_A_CABLE_RESULT_CODE,		/* u8 ETHTOOL_A_CABLE_RESULT_CODE_ */

	__ETHTOOL_A_CABLE_RESULT_CNT,
	ETHTOOL_A_CABLE_RESULT_MAX = (__ETHTOOL_A_CABLE_RESULT_CNT - 1)
};

enum {
	ETHTOOL_A_CABLE_FAULT_LENGTH_UNSPEC,
	ETHTOOL_A_CABLE_FAULT_LENGTH_PAIR,	/* u8 ETHTOOL_A_CABLE_PAIR_ */
	ETHTOOL_A_CABLE_FAULT_LENGTH_CM,	/* u32 */

	__ETHTOOL_A_CABLE_FAULT_LENGTH_CNT,
	ETHTOOL_A_CABLE_FAULT_LENGTH_MAX = (__ETHTOOL_A_CABLE_FAULT_LENGTH_CNT - 1)
};

enum {
	ETHTOOL_A_CABLE_TEST_NTF_STATUS_UNSPEC,
	ETHTOOL_A_CABLE_TEST_NTF_STATUS_STARTED,
	ETHTOOL_A_CABLE_TEST_NTF_STATUS_COMPLETED
};

enum {
	ETHTOOL_A_CABLE_NEST_UNSPEC,
	ETHTOOL_A_CABLE_NEST_RESULT,		/* nest - ETHTOOL_A_CABLE_RESULT_ */
	ETHTOOL_A_CABLE_NEST_FAULT_LENGTH,	/* nest - ETHTOOL_A_CABLE_FAULT_LENGTH_ */
	__ETHTOOL_A_CABLE_NEST_CNT,
	ETHTOOL_A_CABLE_NEST_MAX = (__ETHTOOL_A_CABLE_NEST_CNT - 1)
};

enum {
	ETHTOOL_A_CABLE_TEST_NTF_UNSPEC,
	ETHTOOL_A_CABLE_TEST_NTF_HEADER,	/* nest - ETHTOOL_A_HEADER_* */
	ETHTOOL_A_CABLE_TEST_NTF_STATUS,	/* u8 - _STARTED/_COMPLETE */
	ETHTOOL_A_CABLE_TEST_NTF_NEST,		/* nest - of results: */

	__ETHTOOL_A_CABLE_TEST_NTF_CNT,
	ETHTOOL_A_CABLE_TEST_NTF_MAX = (__ETHTOOL_A_CABLE_TEST_NTF_CNT - 1)
};

/* CABLE TEST TDR */

enum {
	ETHTOOL_A_CABLE_TEST_TDR_CFG_UNSPEC,
	ETHTOOL_A_CABLE_TEST_TDR_CFG_FIRST,		/* u32 */
	ETHTOOL_A_CABLE_TEST_TDR_CFG_LAST,		/* u32 */
	ETHTOOL_A_CABLE_TEST_TDR_CFG_STEP,		/* u32 */
	ETHTOOL_A_CABLE_TEST_TDR_CFG_PAIR,		/* u8 */

	/* add new constants above here */
	__ETHTOOL_A_CABLE_TEST_TDR_CFG_CNT,
	ETHTOOL_A_CABLE_TEST_TDR_CFG_MAX = __ETHTOOL_A_CABLE_TEST_TDR_CFG_CNT - 1
};

enum {
	ETHTOOL_A_CABLE_TEST_TDR_UNSPEC,
	ETHTOOL_A_CABLE_TEST_TDR_HEADER,	/* nest - _A_HEADER_* */
	ETHTOOL_A_CABLE_TEST_TDR_CFG,		/* nest - *_TDR_CFG_* */

	/* add new constants above here */
	__ETHTOOL_A_CABLE_TEST_TDR_CNT,
	ETHTOOL_A_CABLE_TEST_TDR_MAX = __ETHTOOL_A_CABLE_TEST_TDR_CNT - 1
};

/* CABLE TEST TDR NOTIFY */

enum {
	ETHTOOL_A_CABLE_AMPLITUDE_UNSPEC,
	ETHTOOL_A_CABLE_AMPLITUDE_PAIR,         /* u8 */
	ETHTOOL_A_CABLE_AMPLITUDE_mV,           /* s16 */

	__ETHTOOL_A_CABLE_AMPLITUDE_CNT,
	ETHTOOL_A_CABLE_AMPLITUDE_MAX = (__ETHTOOL_A_CABLE_AMPLITUDE_CNT - 1)
};

enum {
	ETHTOOL_A_CABLE_PULSE_UNSPEC,
	ETHTOOL_A_CABLE_PULSE_mV,		/* s16 */

	__ETHTOOL_A_CABLE_PULSE_CNT,
	ETHTOOL_A_CABLE_PULSE_MAX = (__ETHTOOL_A_CABLE_PULSE_CNT - 1)
};

enum {
	ETHTOOL_A_CABLE_STEP_UNSPEC,
	ETHTOOL_A_CABLE_STEP_FIRST_DISTANCE,	/* u32 */
	ETHTOOL_A_CABLE_STEP_LAST_DISTANCE,	/* u32 */
	ETHTOOL_A_CABLE_STEP_STEP_DISTANCE,	/* u32 */

	__ETHTOOL_A_CABLE_STEP_CNT,
	ETHTOOL_A_CABLE_STEP_MAX = (__ETHTOOL_A_CABLE_STEP_CNT - 1)
};

enum {
	ETHTOOL_A_CABLE_TDR_NEST_UNSPEC,
	ETHTOOL_A_CABLE_TDR_NEST_STEP,		/* nest - ETHTTOOL_A_CABLE_STEP */
	ETHTOOL_A_CABLE_TDR_NEST_AMPLITUDE,	/* nest - ETHTOOL_A_CABLE_AMPLITUDE */
	ETHTOOL_A_CABLE_TDR_NEST_PULSE,		/* nest - ETHTOOL_A_CABLE_PULSE */

	__ETHTOOL_A_CABLE_TDR_NEST_CNT,
	ETHTOOL_A_CABLE_TDR_NEST_MAX = (__ETHTOOL_A_CABLE_TDR_NEST_CNT - 1)
};

enum {
	ETHTOOL_A_CABLE_TEST_TDR_NTF_UNSPEC,
	ETHTOOL_A_CABLE_TEST_TDR_NTF_HEADER,	/* nest - ETHTOOL_A_HEADER_* */
	ETHTOOL_A_CABLE_TEST_TDR_NTF_STATUS,	/* u8 - _STARTED/_COMPLETE */
	ETHTOOL_A_CABLE_TEST_TDR_NTF_NEST,	/* nest - of results: */

	/* add new constants above here */
	__ETHTOOL_A_CABLE_TEST_TDR_NTF_CNT,
	ETHTOOL_A_CABLE_TEST_TDR_NTF_MAX = __ETHTOOL_A_CABLE_TEST_TDR_NTF_CNT - 1
};

/* TUNNEL INFO */

enum {
	ETHTOOL_UDP_TUNNEL_TYPE_VXLAN,
	ETHTOOL_UDP_TUNNEL_TYPE_GENEVE,
	ETHTOOL_UDP_TUNNEL_TYPE_VXLAN_GPE,

	__ETHTOOL_UDP_TUNNEL_TYPE_CNT
};

enum {
	ETHTOOL_A_TUNNEL_UDP_ENTRY_UNSPEC,

	ETHTOOL_A_TUNNEL_UDP_ENTRY_PORT,		/* be16 */
	ETHTOOL_A_TUNNEL_UDP_ENTRY_TYPE,		/* u32 */

	/* add new constants above here */
	__ETHTOOL_A_TUNNEL_UDP_ENTRY_CNT,
	ETHTOOL_A_TUNNEL_UDP_ENTRY_MAX = (__ETHTOOL_A_TUNNEL_UDP_ENTRY_CNT - 1)
};

enum {
	ETHTOOL_A_TUNNEL_UDP_TABLE_UNSPEC,

	ETHTOOL_A_TUNNEL_UDP_TABLE_SIZE,		/* u32 */
	ETHTOOL_A_TUNNEL_UDP_TABLE_TYPES,		/* bitset */
	ETHTOOL_A_TUNNEL_UDP_TABLE_ENTRY,		/* nest - _UDP_ENTRY_* */

	/* add new constants above here */
	__ETHTOOL_A_TUNNEL_UDP_TABLE_CNT,
	ETHTOOL_A_TUNNEL_UDP_TABLE_MAX = (__ETHTOOL_A_TUNNEL_UDP_TABLE_CNT - 1)
};

enum {
	ETHTOOL_A_TUNNEL_UDP_UNSPEC,

	ETHTOOL_A_TUNNEL_UDP_TABLE,			/* nest - _UDP_TABLE_* */

	/* add new constants above here */
	__ETHTOOL_A_TUNNEL_UDP_CNT,
	ETHTOOL_A_TUNNEL_UDP_MAX = (__ETHTOOL_A_TUNNEL_UDP_CNT - 1)
};

enum {
	ETHTOOL_A_TUNNEL_INFO_UNSPEC,
	ETHTOOL_A_TUNNEL_INFO_HEADER,			/* nest - _A_HEADER_* */

	ETHTOOL_A_TUNNEL_INFO_UDP_PORTS,		/* nest - _UDP_TABLE */

	/* add new constants above here */
	__ETHTOOL_A_TUNNEL_INFO_CNT,
	ETHTOOL_A_TUNNEL_INFO_MAX = (__ETHTOOL_A_TUNNEL_INFO_CNT - 1)
};

/* generic netlink info */
#define ETHTOOL_GENL_NAME "ethtool"
#define ETHTOOL_GENL_VERSION 1

#define ETHTOOL_MCGRP_MONITOR_NAME "monitor"

#endif /* _UAPI_LINUX_ETHTOOL_NETLINK_H_ */