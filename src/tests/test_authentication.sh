#!/bin/bash
source tests/common.sh
wait_for_qemu_start

# Listing partitions should work because security_state < 3
$PB part -l

result_code=$?

if [ $result_code -ne 0 ];
then
    test_end_error
fi

# Lock device
$PB dev -w -y

result_code=$?

if [ $result_code -ne 0 ];
then
    test_end_error
fi

# Listing partitions should not work because security_state == 3
$PB part -l
result_code=$?

if [ $result_code -ne 255 ];
then
    test_end_error
fi

# Authenticate
$PB dev -a -n 0 -f tests/test_auth_cookie -s RSA4096:sha256
result_code=$?

if [ $result_code -ne 0 ];
then
    test_end_error
fi

$PB part -l
result_code=$?

if [ $result_code -ne 0 ];
then
    test_end_error
fi

test_end_ok
