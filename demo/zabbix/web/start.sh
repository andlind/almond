#!/bin/sh
php-fpm84 -D
nginx -g 'daemon off;'
