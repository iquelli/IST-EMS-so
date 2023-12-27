#include "workers.h"

// Handlers

// TODO: these all should read the pipe and perform the intended action,
//       replying to client when necessary

int ems_setup_handler(int session_id, client_t *client);

int ems_quit_handler(client_t *client);

int ems_create_handler(client_t *client);

int ems_reserve_handler(client_t *client);

int ems_show_handler(client_t *client);

int ems_list_handler(client_t *client);