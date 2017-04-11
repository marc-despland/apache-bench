#!/bin/bash


if [ '$1' == 'prefork' or  '$1' == 'event' or '$1' == 'worker' ] 
then
	ln -s /etc/apache2/mod-available/mpm_$1.conf /etc/apache2/mods-enabled/mpm.conf
	ln -s /etc/apache2/mod-available/mpm_$1.load /etc/apache2/mods-enabled/mpm.load
fi