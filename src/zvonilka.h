/** zvonilka: public interface */

#pragma once
#include <phiola.h>

#define ZVON_CORE_VER  2

typedef struct zvon_call zvon_call;


/* Relay */

enum {
	ZVON_REL_NEW_CLIENT = 1,
	ZVON_REL_DISCONNECT = 2,
	ZVON_REL_NEW_CALL = 4,
};

typedef struct zvon_relay_ctl zvon_relay_ctl;
struct zvon_relay_ctl {
	void (*connection)(void *obj, uint flags);
};

struct zvon_relay_conf {
	uint		port;

	const zvon_relay_ctl*	controller;
	void*					opaque;
};

typedef struct zvon_relay zvon_relay;
typedef struct zvon_relay_if zvon_relay_if;
struct zvon_relay_if {
	zvon_relay* (*listen)(struct zvon_relay_conf *conf);
	void (*close)();
};


/* Connection */

enum {
	ZVON_CONN_CONNECTED = 1,
	ZVON_CONN_DISCONNECTED = 2,
};

/** Controller */
typedef struct zvon_ctl zvon_ctl;
struct zvon_ctl {
	void (*connection)(void *obj, uint flags);

	/** Called when a new call is established */
	void (*open)(void *obj, zvon_call *c);

	/** Called when a call ends */
	void (*close)(void *obj, zvon_call *c);

	int (*process)(void *obj, zvon_call *c);
};

/** Connection configuration data */
struct zvon_conn_conf {
	char		name[128];
	u_char		ip[16];
	uint		port;

	const char*	audio_module;
	uint		mic_dev_index, play_dev_index;
	uint		buffer_length_msec;
	uint		bitrate_kbps;
	uint		bandwidth_khz;
	uint		gain_db;

	const zvon_ctl*	controller;
	void*			opaque;
};

enum ZVON_CONN {
	ZVON_CONN_STOP = 1,
};

typedef struct zvon_conn zvon_conn;
typedef struct zvon_conn_if zvon_conn_if;
/** Connection interface */
struct zvon_conn_if {
	/**
	sig: enum ZVON_CONN */
	int (*sig)(uint sig);
	zvon_conn* (*connect)(struct zvon_conn_conf *conf);
	void (*close)(zvon_conn *c);
	void (*call)(zvon_conn *c, const char *target);
};


/* Call */

enum ZVON_CLS {
	ZVON_CLS_NONE,
	ZVON_CLS_ESTABLISHED, // both channels are active
	ZVON_CLS_ERR,
	ZVON_CLS_FIN, // one channel is closed

	ZVON_CLS_INCOMING = 0x10, // incoming call
};

enum ZVON_CALL {
	ZVON_CALL_STOP = 1,
	ZVON_CALL_FIN,
	ZVON_CALL_ERR,
};

enum ZVON_CLG {
	ZVON_CLG_PEER_IP = 1, // char*
};

/** Call interface */
typedef struct zvon_call_if zvon_call_if;
struct zvon_call_if {
	void (*close)(zvon_call *c);

	/**
	sig: enum ZVON_CALL */
	int (*sig)(zvon_call *c, uint sig);

	/**
	Return enum ZVON_CLS */
	uint (*state)(zvon_call *c);

	/**
	flags: enum ZVON_CLG */
	void* (*get)(zvon_call *c, uint flags);
};
