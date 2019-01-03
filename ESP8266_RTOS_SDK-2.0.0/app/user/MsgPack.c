#include "user_config.h"
#include "WlinkWifi.h"
#include <msgpack.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

const char* SN_NAME = "sn";
const char* SENDER_NAME = "sender";
const char* RECEIVER_NAME = "receiver";
const char* BEHAVOIR_NAME = "behavoir";
const char* UUID_NAME = "uuid";
const char* BODY_NAME = "body";

const char* STATE_NAME = "state";
const char* SERVICES_NAME = "services";

const char* ENDPOINT_NAME = "endpoint";
const char* SVCID_NAME = "svcId";
const char* ATTRID_NAME = "attrId";
const char* ATTRVALUE_NAME = "attrValue";

const int WLINK_INFO_SIZE = 6;
/**************************************************************/

void user_print(char const* buf, unsigned int len) {
	size_t i = 0;
	for (; i < len; ++i)
		printf("%02x ", 0xff & buf[i]);
	printf("\n");
}

void TakeOutStr(msgpack_object_kv* p, const char* in, char* out, char* taked, char t) {
	bool isTaked = *taked & t;
	if (isTaked == false &&
		p->key.via.str.size == strlen(in) &&
		strncmp(p->key.via.str.ptr, in, p->key.via.str.size) == 0 &&
		p->val.type == MSGPACK_OBJECT_STR) {
		//memcpy(out, p->val.via.str.ptr, p->val.via.str.size);
		//*(out + p->val.via.str.size) = '\0';
		snprintf(out, p->val.via.str.size + 1, "%s",p->val.via.str.ptr);
		*taked |= t;
	}
}

void TakeOutInt(msgpack_object_kv* p, const char* in, int* out, char* taked, char t) {
	bool isTaked = *taked & t;
	if (isTaked == false &&
		p->key.via.str.size == strlen(in) &&
		strncmp(p->key.via.str.ptr, in, p->key.via.str.size) == 0 &&
		p->val.type == MSGPACK_OBJECT_POSITIVE_INTEGER) {
		*out = p->val.via.i64;
		*taked |= t;
	}
}

bool TakeOutArray(msgpack_object_kv* p, const char* in, msgpack_object* out, char* taked, char t) {
	bool isTaked = *taked & t;
	if (isTaked == false &&
		p->key.via.str.size == strlen(in) &&
		strncmp(p->key.via.str.ptr, in, p->key.via.str.size) == 0 &&
		p->val.type == MSGPACK_OBJECT_ARRAY) {
		*out = p->val;
		*taked |= t;
		return true;
	}
	return false;
}

bool TakeOutMap(msgpack_object_kv* p, const char* in, msgpack_object* out, char* taked, char t) {
	bool isTaked = *taked & t;
	if (isTaked == false &&
		p->key.via.str.size == strlen(in) &&
		strncmp(p->key.via.str.ptr, in, p->key.via.str.size) == 0 &&
		p->val.type == MSGPACK_OBJECT_MAP) {
		*out = p->val;
		*taked |= t;
		return true;
	}
	return false;
}

void UnPackAppAnnounceInfo(AppAnnounceInfo* info, msgpack_object* obj) {
	if (obj->type != MSGPACK_OBJECT_ARRAY ||
		obj->via.array.size != 1 ||
		obj->via.array.ptr[0].type != MSGPACK_OBJECT_MAP) {
		return;
	}

	msgpack_object svcObj;
	{
		msgpack_object_kv* p = obj->via.array.ptr[0].via.map.ptr;
		msgpack_object_kv* const pend = obj->via.array.ptr[0].via.map.ptr + obj->via.array.ptr[0].via.map.size;

		char taked = 0;
		for (; p < pend; ++p) {
			TakeOutStr(p, STATE_NAME, info->state, &taked, 1);
			TakeOutMap(p, SERVICES_NAME, &svcObj, &taked, 1 << 1);
		}
	}

	{
		msgpack_object_kv* p = svcObj.via.map.ptr;
		msgpack_object_kv* const pend = svcObj.via.map.ptr + svcObj.via.map.size;
		info->svcNum = svcObj.via.map.size;
		char taked = 0;
		int i = 0;
		for (i; p < pend; ++p, i++) {
			memcpy(info->services[i][0], p->key.via.str.ptr, p->key.via.str.size);
			memcpy(info->services[i][1], p->val.via.str.ptr, p->val.via.str.size);
		}
	}
}

void DeserializeWlinkInfo(const char* str, const int len, WlinkInfo* info) {
	msgpack_zone zone;
	msgpack_object obj;

	msgpack_zone_init(&zone, 1024);

	msgpack_unpack(str, len, NULL, &zone, &obj);

	if (obj.type != MSGPACK_OBJECT_MAP ||
		obj.via.map.size == 0) {
		return;
	}

	msgpack_object_kv* p = obj.via.map.ptr;
	msgpack_object_kv* const pend = obj.via.map.ptr + obj.via.map.size;
	char taked = 0;
	for (; p < pend; ++p) {
		TakeOutStr(p, SN_NAME, info->sn, &taked, 1);
		TakeOutStr(p, SENDER_NAME, info->sender, &taked, 1 << 1);
		TakeOutStr(p, RECEIVER_NAME, info->receiver, &taked, 1 << 2);
		TakeOutInt(p, BEHAVOIR_NAME, &info->behavoir, &taked, 1 << 3);
		TakeOutStr(p, UUID_NAME, info->uuid, &taked, 1 << 4);

		msgpack_object o;
		if (TakeOutArray(p, BODY_NAME, &o, &taked, 1 << 5)) {
			if (info->behavoir == WLINK_BEHAVIOR_APP_ANNOUNCE) {
				info->body.appAnno = (AppAnnounceInfo*)malloc(sizeof(info->body.appAnno));
				UnPackAppAnnounceInfo(info->body.appAnno, &o);

				free(info->body.appAnno);
			}
		}
	}

	msgpack_zone_destroy(&zone);
}

void PackAppAnnounceInfo(AppAnnounceInfo* info, msgpack_packer *pk) {
	msgpack_pack_array(pk, 1);

	msgpack_pack_map(pk, 2);

	{
		int keySize = strlen(STATE_NAME);
		msgpack_pack_str(pk, keySize);
		msgpack_pack_str_body(pk, STATE_NAME, keySize);

		int valueSize = strlen(info->state);
		msgpack_pack_str(pk, valueSize);
		msgpack_pack_str_body(pk, info->state, valueSize);
	}

	{
		int keySize = strlen(SERVICES_NAME);
		msgpack_pack_str(pk, keySize);
		msgpack_pack_str_body(pk, SERVICES_NAME, keySize);

		msgpack_pack_map(pk, info->svcNum);
		int i = 0;
		for (i; i < info->svcNum; i++) {
			int keySize = strlen(info->services[i][0]);
			msgpack_pack_str(pk, keySize);
			msgpack_pack_str_body(pk, info->services[i][0], keySize);

			int valueSize = strlen(info->services[i][1]);
			msgpack_pack_str(pk, valueSize);
			msgpack_pack_str_body(pk, info->services[i][1], valueSize);
		}
	}
}

void PackSvcAttrReport(SvcAttrReportInfo* info, int size, msgpack_packer *pk) {
	msgpack_pack_array(pk, size);

	int i = 0;
	for (i; i < size; i++) {
		msgpack_pack_map(pk, 4);
		{
			int keySize = strlen(ENDPOINT_NAME);
			msgpack_pack_str(pk, keySize);
			msgpack_pack_str_body(pk, ENDPOINT_NAME, keySize);

			msgpack_pack_int32(pk, info->endpoint);
		}

		{
			int keySize = strlen(SVCID_NAME);
			msgpack_pack_str(pk, keySize);
			msgpack_pack_str_body(pk, SVCID_NAME, keySize);

			int valueSize = strlen(info->svcId);
			msgpack_pack_str(pk, valueSize);
			msgpack_pack_str_body(pk, info->svcId, valueSize);
		}

		{
			int keySize = strlen(ATTRID_NAME);
			msgpack_pack_str(pk, keySize);
			msgpack_pack_str_body(pk, ATTRID_NAME, keySize);

			int valueSize = strlen(info->attrId);
			msgpack_pack_str(pk, valueSize);
			msgpack_pack_str_body(pk, info->attrId, valueSize);
		}

		{
			int keySize = strlen(ATTRVALUE_NAME);
			msgpack_pack_str(pk, keySize);
			msgpack_pack_str_body(pk, ATTRVALUE_NAME, keySize);

			int valueSize = strlen(info->attrValue);
			msgpack_pack_str(pk, valueSize);
			msgpack_pack_str_body(pk, info->attrValue, valueSize);
		}
	}
}

void SerializeWlinkInfo(WlinkInfo* info, char* str, int* len) {

	msgpack_sbuffer buffer;
	msgpack_packer pk;
	msgpack_sbuffer_init(&buffer);
	msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

	msgpack_pack_map(&pk, WLINK_INFO_SIZE);

	{
		int keySize = strlen(SN_NAME);
		msgpack_pack_str(&pk, keySize);
		msgpack_pack_str_body(&pk, SN_NAME, keySize);

		int valueSize = strlen(info->sn);
		msgpack_pack_str(&pk, valueSize);
		msgpack_pack_str_body(&pk, info->sn, valueSize);
	}
	{
		int keySize = strlen(SENDER_NAME);
		msgpack_pack_str(&pk, keySize);
		msgpack_pack_str_body(&pk, SENDER_NAME, keySize);

		int valueSize = strlen(info->sender);
		msgpack_pack_str(&pk, valueSize);
		msgpack_pack_str_body(&pk, info->sender, valueSize);
	}
	{
		int keySize = strlen(RECEIVER_NAME);
		msgpack_pack_str(&pk, keySize);
		msgpack_pack_str_body(&pk, RECEIVER_NAME, keySize);

		int valueSize = strlen(info->receiver);
		msgpack_pack_str(&pk, valueSize);
		msgpack_pack_str_body(&pk, info->receiver, valueSize);
	}
	{
		int keySize = strlen(BEHAVOIR_NAME);
		msgpack_pack_str(&pk, keySize);
		msgpack_pack_str_body(&pk, BEHAVOIR_NAME, keySize);

		msgpack_pack_int32(&pk, info->behavoir);
	}
	{
		int keySize = strlen(UUID_NAME);
		msgpack_pack_str(&pk, keySize);
		msgpack_pack_str_body(&pk, UUID_NAME, keySize);

		int valueSize = strlen(info->uuid);
		msgpack_pack_str(&pk, valueSize);
		msgpack_pack_str_body(&pk, info->uuid, valueSize);
	}
	{
		int keySize = strlen(BODY_NAME);
		msgpack_pack_str(&pk, keySize);
		msgpack_pack_str_body(&pk, BODY_NAME, keySize);

		switch (info->behavoir) {
		case WLINK_BEHAVIOR_APP_ANNOUNCE:
		{
			PackAppAnnounceInfo(info->body.appAnno, &pk);
		}break;
		case WLINK_BEHAVIOR_SVC_REPORT:
		{
			PackSvcAttrReport(info->body.svcAttrReport, sizeof(info->body.svcAttrReport), &pk);
		}break;
		default:
			break;
		}

	}

	memcpy(str, buffer.data, buffer.size);
	*len = buffer.size;

//	user_print(buffer.data, buffer.size);
//
//	msgpack_zone zone;
//	msgpack_object obj;
//	msgpack_zone_init(&zone, 1024);
//
//	msgpack_unpack(buffer.data, buffer.size, NULL, &zone, &obj);
//
//	char jsonOut[1024] = { 0 };
//	int jsonSize = 1024;
//	msgpack_object_print_buffer(jsonOut, jsonSize, obj);
//	printf("%s\n", jsonOut);
//	msgpack_zone_destroy(&zone);

	msgpack_sbuffer_destroy(&buffer);
}



/****************************************************************************/
void DevStatus(char * buf, int * len, char * status)
{
	WlinkInfo info;
	AppAnnounceInfo annoInfo = {
		"1",3,
		{
			{ "1","10" },{ "2","11" },{ "3","20" }
		}
	};

	if(strcmp(status , DEV_LEAVE_LINE) == 0)
	{
		strcpy(annoInfo.state, DEV_LEAVE_LINE);
	}

	strcpy(info.sn, "1");
	strcpy(info.sender, "600194220129");
	strcpy(info.receiver, "");
	strcpy(info.uuid, "600194220129");
	info.behavoir = WLINK_BEHAVIOR_APP_ANNOUNCE;
	info.body.appAnno = &annoInfo;

	SerializeWlinkInfo(&info, buf, len);
}



unsigned char  mqtt_sent_buf[256] = { 0 };
int mqtt_sent_len = 0;

void Publish(char* str, int len) {
    memcpy(mqtt_sent_buf, str, len);
    mqtt_sent_len = len;
}

void DevDataSend(char * buf, int * len)
{
	WlinkInfo info;
	SvcAttrReportInfo attrInfo[] = {
		{ 1,"10","1","" },
		{ 1,"10","2","" },
		{ 2,"11","2","" },
		{ 3,"20","3","" }
	};

	strcpy(info.sn, "1");
	strcpy(info.sender, "600194220129");
	strcpy(info.receiver, "");
	strcpy(info.uuid, "600194220129");
	info.behavoir = WLINK_BEHAVIOR_SVC_REPORT;
	info.body.svcAttrReport = attrInfo;


	char tem_attrValue[10];
	sprintf(attrInfo[0].attrValue,"%d.%d",(mqtt_sent_buf[6]-44),mqtt_sent_buf[7]);
	printf("temperature=%s\n", attrInfo[0].attrValue);

	sprintf(attrInfo[1].attrValue,"%d",mqtt_sent_buf[8]+7);
	printf("humidity=%s\n",attrInfo[1].attrValue);

	if(mqtt_sent_buf[19] == 0)
	{
		sprintf(attrInfo[2].attrValue,"%d",mqtt_sent_buf[20]-10);
	}
	else
	{
		sprintf(attrInfo[2].attrValue,"%d%d",mqtt_sent_buf[19],mqtt_sent_buf[20]-10);
	}
	printf("PM2.5=%s\n",attrInfo[2].attrValue);


	sprintf(attrInfo[3].attrValue,"%d.%d%d",mqtt_sent_buf[9],mqtt_sent_buf[10],mqtt_sent_buf[11]);
	printf("formaldehyde=%s\n",attrInfo[3].attrValue);

	SerializeWlinkInfo(&info, buf, len);
}
