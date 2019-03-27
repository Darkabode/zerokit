#
# Makefile for ZController
#

include $(POCO_BASE)/build/rules/global

objects = main delayedWorker globals

SYSLIBS += -L/usr/local/lib -L/usr/local/lib/mysql -L/usr/lib/mysql -L/usr/mysql/lib/mysql
INCLUDE += -I/usr/local/include/mysql/ -I/usr/include/mysql/ -I/usr/mysql/include/mysql
SYSFLAGS += -DTHREADSAFE -DNO_TCL

SYSLIBS += -lGeoIP
SYSLIBS += -lmysqlclient

target         = zcontroller
target_version = 1
target_libs    = PocoUtil PocoNet PocoData PocoDataMySQL PocoXML PocoJSON PocoFoundation 

include $(POCO_BASE)/build/rules/exec
