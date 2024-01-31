#!/bin/bash

output_path=$1
input_string=$2

mkdir -p $(dirname "$output_path")
rm -f $output_path
hadd $output_path ${input_string//,/ }
