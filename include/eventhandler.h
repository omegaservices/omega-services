#ifndef __EVENTHANDLER_H__
#define __EVENTHANDLER_H__


//events are timed.
#define EVENT_TYPE_TIMED      0

//Event can be triggured outside the core.
#define EVENT_TYPE_CALLABLE   1

//Event cant be triggured other other then 
//by the core
#define EVENT_TYPE_UNCALLABLE 2


typedef enum { 
	EVENT_SUCCESS = 0,
	EVENT_HALT,
	EVENT_ERROR,
	EVENT_FATAL,
	EVENT_SKIP,
	EVENT_RUN_LATER
} event_return_t;


typedef union { 
	void   * ptr;
	Socket * sock;
} event_args_t;

typedef struct { 
	char file[MAXPATH];
	char func[MAXFUNC];
	int  lineno;
	int priority;
	event_return_t  (*handler)(event_args_t *);
} event_t;

typedef struct {
	int runtime;
	int type;	
	dlink_list events;
} event_chain_t;


dlink_list events;

#define event(x,arg) event_dispatch(_A_, x, arg)

#define event_dispatch(evt,arg)  _event_dispatch(_A_, evt, arg)
#define event_add(evt, fnct)     _event_add(_A_, evt, fnct)
#define event_remove(evnt, fnct) _event_remove(_A_, evt, fnct);

int _event_dispatch     (char *, int, char *, char *, event_args_t *);
int _event_add          (char *, int, char *, char *, event_return_t  (*)(event_args_t *));
int _event_remove       (char *, int, char *, char *, event_return_t (*)(event_args_t *));

#endif //__EVENTHANDLER_H__