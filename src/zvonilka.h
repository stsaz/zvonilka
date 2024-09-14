/** zvonilka: public interface */

#pragma once
#include <phiola.h>

typedef struct zvon_call zvon_call;


/* Connection */

/** Controller */
typedef struct zvon_ctl zvon_ctl;
struct zvon_ctl {
	/** Called when a new call is established */
	void (*open)(void *obj, zvon_call *c);

	/** Called when a call ends */
	void (*close)(void *obj, zvon_call *c);

	int (*process)(void *obj, zvon_call *c);
};

/** Connection configuration data */
struct zvon_conn_conf {
	u_char		ip[16];
	uint		port;

	const char*	audio_module;
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
	zvon_conn* (*listen)(struct zvon_conn_conf *conf);
	void (*close)(zvon_conn *c);
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
