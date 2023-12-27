#ifndef __WORKERS_H__
#define __WORKERS_H__

#include "common/io.h"

// Handler functions for client requests.

// TODO: Add whatever params they need and comment function

int ems_setup_handler(int session_id, client_t *client);

int ems_quit_handler(client_t *client);

int ems_create_handler(client_t *client);

int ems_reserve_handler(client_t *client);

int ems_show_handler(client_t *client);

int ems_list_handler(client_t *client);

#endif  // __WORKERS_H__