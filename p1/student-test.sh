#!/bin/sh

# get all files ending with .out from ./jobs/
# and compares them with their respective .result also in ./jobs/
# if the output is the same, print "Test _ SUCCESS" in green,
# otherwise print "Test _ FAILED" in red

# Set the directory where the files are located
directory="./student-tests/"

# Iterate through each .out file in the directory
for outfile in "${directory}"*.out; do
    # Get the corresponding .result file
    resultfile="${outfile%.out}.result"

    # Check if the result file exists
    if [ -e "${resultfile}" ]; then
        # Compare the content of the .out and .result files
        if cmp -s "${outfile}" "${resultfile}"; then
            # Print success message in green
            echo -e "\e[32mTest $(basename "${outfile}") SUCCESS\e[0m"
        else
            # Print failure message in red
            echo -e "\e[31mTest $(basename "${outfile}") FAILED\e[0m"
            echo "Differences:"
            diff "${outfile}" "${resultfile}"
        fi
    else
        # Print message if corresponding .result file is missing
        echo "Result file not found for $(basename "${outfile}")"
    fi
done