#ifndef __WORKERS_H__
#define __WORKERS_H__

#include "common/io.h"

// Handler functions for client requests on the server side.

int ems_setup_handler(int session_id, client_t *client);

int ems_create_handler(client_t *client, unsigned int event_id, size_t num_rows, size_t num_cols);

int ems_reserve_handler(client_t *client, unsigned int event_id, size_t num_seats, size_t *xs, size_t *ys);

int ems_show_handler(client_t *client, unsigned int event_id);

int ems_list_handler(client_t *client);

#endif  // __WORKERS_H__