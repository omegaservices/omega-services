#include "stdinclude.h"
#include "appinclude.h"

/*********************************************************/

static void core_enable_core_dump() 
{
#if defined(RLIMIT_CORE) && defined(HAVE_SYS_RESOURCE_H)
    // Make sure we can dump a core
    struct rlimit   rlim;
    int ret = getrlimit (RLIMIT_CORE, &rlim);
    if (ret == 0)
    {
        if (rlim.rlim_cur == 0)
        {
            log_message(LOG_DEBUG, "Core limit 0, attempting to set to %d\n", (int)rlim.rlim_max);

            rlim.rlim_cur    = rlim.rlim_max;
            setrlimit (RLIMIT_CORE, &rlim);
        }
    }
#endif
}

/*********************************************************/

void core_parse_opts(int argc, char ** argv, char ** env) { 
    int c;

    while ((c = getopt (argc, argv, "bnc:d:")) != -1)
    {
        switch (c)
        {
            case 'c':
                if (optarg) 
                {
                    if ((optarg[strlen(optarg) - 4] == '.') 
                        && optarg[strlen(optarg) - 3] == 'c') 
                    {
                        if ((*optarg == '/') || (*optarg == '.'))
                            strlcpy (core.settings.config_file, optarg, sizeof(core.settings.config_file));
                        else
                            sprintf (core.settings.config_file, "%s/%s", CONFIG_DIR, optarg);
                    }
                } 
                else
                    fprintf(stderr,"Invalid config file specified with -c (%s) option skipping.\n", optarg);
                    break;
            case 'd':
                if (optarg) core.debug = atoi(optarg);
                break;
            case 'n':
                fprintf(stderr, "Running in nofork mode\n");
                core.nofork = TRUE;
                break;
        }
    }
    core.cmdline.argv = argv;
    core.cmdline.argc = argc;
    core.cmdline.env  = env;
}



/*********************************************************/

void core_init() {
    sync_state = SYNC_STATE_STARTUP;

    log_message(LOG_NOTICE, "Config File: %s\n", core.settings.config_file);

    config_load(core.settings.config_file);

    load_config_values();

    core_enable_core_dump();

    if (!core.nofork) daemonize();

    //Initialize our signal handler
    sighandler_init();
    event_init();

    atexit(core_cleanup);


    mq_load_module("select", MOD_TYPE_SOCKET); //inject into modq
    
    modules_init(); // Run our modq to handle config load modules
                    // This will load all module orders, PRE -> STD -> POST
   
}

/*********************************************************/

int core_connect_uplink()
{
    int ret = 0;

    core.socket = socket_new();
    core.socket->flags |= SOCK_UPLINK;
    
    log_message(LOG_INFO, "Connecting to: %s:%d", core.settings.con_host, core.settings.con_port);

    ret = se_connect(core.socket, core.settings.con_host, 
                core.settings.local_ip, core.settings.con_port);
    if (ret < 0)
    {
        log_message(LOG_ERROR,"Unable to connect to %s:%d", core.settings.con_host, 
            core.settings.con_port);        
        socket_free(core.socket);
    }   
    else
        socket_addto_list(core.socket);

    return ret;
}

/*********************************************************/

void core_run(void) { 
    int ret    = 0;
    
    event_dispatch("STARTUP", NULL);

    se_startup();

    if (core_connect_uplink() < 0)
        core_exit(0);
    
    log_message(LOG_NOTICE, "Begining normal runtime.");
    sync_state = SYNC_STATE_NORMAL;
    while (sync_state != SYNC_STATE_SHUTDOWN)
        core_once_around();

    //We are in a weird sync_state shutdown.
    core_exit(0); 
}


/*********************************************************/

void core_once_around(void)
{
    se_receive();
}

/*********************************************************/

void core_cleanup(void) {
    FILE *fp;

    log_message(LOG_NOTICE, "Cleaning up core");
    destroy_config_tree();

    se_cleanup();
    modules_purge();

    //We have a pidfile last we need to clean it up
    if (!core.nofork && (fp = fopen(core.settings.pid_file, "r"))) 
    {
        fclose(fp);
        unlink(core.settings.pid_file);
    }

    //Close our logs last, we dont want them reopened.
    log_close();
}

/*********************************************************/

void core_exit(int code) {
    log_message(LOG_NOTICE, "Exitting on code: %d", code);
    //core_cleanup();
    exit(code);
}

/*********************************************************/

int core_reload() 
{
    log_message(LOG_NOTICE, "Reloading configuration files");
    destroy_config_tree();
    config_load(core.settings.config_file);
    load_config_values();
    return TRUE;
}

