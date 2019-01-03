#ifndef __WLINK_WIFI_H
#define __WLINK_WIFI_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	WLINK_BEHAVIOR_APP_SCAN = 1,
	WLINK_BEHAVIOR_APP_ANNOUNCE = 2,
	WLINK_BEHAVIOR_SVC_CMD = 101,
	WLINK_BEHAVIOR_SVC_CMD_RES = 102,
	WLINK_BEHAVIOR_SVC_REPORT = 103,
	WLINK_BEHAVIOR_SVC_FAST_REPORT = 104,
	WLINK_BEHAVIOR_SVC_ALARM = 105,
	WLINK_BEHAVIOR_SVC_READ_ATTR = 106,
	WLINK_BEHAVIOR_SVC_READ_ATTR_RES = 107,
	WLINK_BEHAVIOR_SVC_WRITE_ATTR = 108,
	WLINK_BEHAVIOR_SVC_WRITE_ATTR_RES = 109,

	WLINK_BEHAVIOR_FUTURE_QUERY = 201,
	WLINK_BEHAVIOR_FUTURE_QUERY_RES = 202,
	WLINK_BEHAVIOR_FUTURE_UPDATE = 203,
	WLINK_BEHAVIOR_FUTURE_UPDATE_RES = 204,
	WLINK_BEHAVIOR_FUTURE_REPORT = 205,

	WLINK_BEHAVIOR_SCHEDULER_CREATE = 301,
	WLINK_BEHAVIOR_SCHEDULER_CREATE_RES = 302,
	WLINK_BEHAVIOR_SCHEDULER_QUERY = 303,
	WLINK_BEHAVIOR_SCHEDULER_QUERY_RES = 304,
	WLINK_BEHAVIOR_SCHEDULER_UPDATE = 305,
	WLINK_BEHAVIOR_SCHEDULER_UPDATE_RES = 306,
	WLINK_BEHAVIOR_SCHEDULER_DELETE = 307,
	WLINK_BEHAVIOR_SCHEDULER_DELETE_RES = 308,

}WkBehavior; //cmd

#if 1

typedef struct {
	char state[2];
	int svcNum;
	char services[3][2][3];
}AppAnnounceInfo;

typedef struct {
	int endpoint;
	char svcId[3];
	char attrId[3];
	char attrValue[8];
}SvcAttrReportInfo;

typedef union {
	AppAnnounceInfo *appAnno;
	SvcAttrReportInfo *svcAttrReport;
}WlinkBody;

typedef struct {
	char sn[6];
	char sender[32];
	char receiver[32];
	int behavoir;
	char uuid[32];
	WlinkBody body;
}WlinkInfo;

#endif

#ifdef __cplusplus
}
#endif

#endif
