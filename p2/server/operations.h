#ifndef SERVER_OPERATIONS_H
#define SERVER_OPERATIONS_H

#include <stddef.h>

/// Initializes the EMS state.
/// @param delay_us Delay in microseconds.
/// @return 0 if the EMS state was initialized successfully, 1 otherwise.
int ems_init(unsigned int delay_us);

/// Destroys the EMS state.
int ems_terminate();

/// Creates a new event with the given id and dimensions.
/// @param event_id Id of the event to be created.
/// @param num_rows Number of rows of the event to be created.
/// @param num_cols Number of columns of the event to be created.
/// @return 0 if the event was created successfully, 1 otherwise.
int ems_create(unsigned int event_id, size_t num_rows, size_t num_cols);

/// Creates a new reservation for the given event.
/// @param event_id Id of the event to create a reservation for.
/// @param num_seats Number of seats to reserve.
/// @param xs Array of rows of the seats to reserve.
/// @param ys Array of columns of the seats to reserve.
/// @return 0 if the reservation was created successfully, 1 otherwise.
int ems_reserve(unsigned int event_id, size_t num_seats, size_t* xs, size_t* ys);

/// Gets information about a specific event.
/// @param event_id The id of the event to retrive information.
/// @param cols The variable to store the number of columns.
/// @param data The variable to store the data.
/// @param rows The variable to store the number of rows.
/// @return 0 if successful, 1 otherwise.
int get_event_info(unsigned int event_id, size_t* cols, unsigned int** data, size_t* rows);

/// Gets all event ids.
/// @param data Variable to store event ids.
/// @param num_events Variable to store number of events.
/// @return 0 if successful, 1 otherwise.
int get_events(unsigned int** data, size_t* num_events);

/// Prints the given event.
/// @param event_id Id of the event to print.
/// @return 0 if the event was printed successfully, 1 otherwise.
int ems_show(unsigned int event_id);

/// Prints all the events.
/// @return 0 if the events were printed successfully, 1 otherwise.
int ems_list_events();

#endif  // SERVER_OPERATIONS_H
