#!/bin/bash

# This is a simple script that uses a here document
# to pass a list of users to the `sort` command

sort << EOF
John Doe
Jane Doe
Jim Smith
Jane Smith
EOF
