#!/bin/bash

# Set the paths and names
jobs_directory="jobs"
client_executable="./client/client"
client_request_pipe_prefix="REQ"
client_response_pipe_prefix="RESP"
pipe_counter=1

# Iterate through all .jobs files in the specified directory
for job_file in "$jobs_directory"/*.jobs; do
    if [ -f "$job_file" ]; then
        # Construct the pipe names
        request_pipe="$client_request_pipe_prefix$pipe_counter"
        response_pipe="$client_response_pipe_prefix$pipe_counter"

        # Run the client command with the current .jobs file and pipe names
        "$client_executable" "$request_pipe" "$response_pipe" pipes "$job_file"

        # Increment the pipe counter
        ((pipe_counter++))
    fi
done