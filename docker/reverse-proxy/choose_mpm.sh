#!/bin/bash

case "$1" in
	"prefork")
		rm /etc/apache2/mods-enabled/mpm.conf
		rm /etc/apache2/mods-enabled/mpm.load
		ln -s /etc/apache2/mod-available/mpm_$1.conf /etc/apache2/mods-enabled/mpm.conf
		ln -s /etc/apache2/mod-available/mpm_$1.load /etc/apache2/mods-enabled/mpm.load
		;;
	"event")
		rm /etc/apache2/mods-enabled/mpm.conf
		rm /etc/apache2/mods-enabled/mpm.load
		ln -s /etc/apache2/mod-available/mpm_$1.conf /etc/apache2/mods-enabled/mpm.conf
		ln -s /etc/apache2/mod-available/mpm_$1.load /etc/apache2/mods-enabled/mpm.load
		;;
	"worker")
		rm /etc/apache2/mods-enabled/mpm.conf
		rm /etc/apache2/mods-enabled/mpm.load
		ln -s /etc/apache2/mod-available/mpm_$1.conf /etc/apache2/mods-enabled/mpm.conf
		ln -s /etc/apache2/mod-available/mpm_$1.load /etc/apache2/mods-enabled/mpm.load
		;;
	*)
		echo Usage : $0 [prefork|event|worker]
		;;
esac