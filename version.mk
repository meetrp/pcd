#
# Makfile for generating version

PCD_VERSION := $(shell git describe --tags 2> /dev/null)
PCD_VERSION_DATE := $(shell git log -1 --format=%cd 2> /dev/null)
PCD_BRANCH := $(shell git symbolic-ref HEAD 2> /dev/null | cut -d'/' -f3-)
PCD_BUILD_DATE := $(shell date -R)
PCD_VERSION_H := $(CURDIR)/pcd/include/pcd_version.h

PCD_TAG_VERSION := "v1.2.1"

generate_version:
	@echo "/*******************************************************************" > $(PCD_VERSION_H);
	@echo "*                AUTO GENERATED; DO NOT MODIFY                    *" >> $(PCD_VERSION_H);
	@echo "*******************************************************************/" >> $(PCD_VERSION_H);
	@echo "" >> $(PCD_VERSION_H);
	@echo "#ifndef __PCD_VERSION_H__" >> $(PCD_VERSION_H);
	@echo "#define __PCD_VERSION_H__" >> $(PCD_VERSION_H);
	@echo "" >> $(PCD_VERSION_H);
	@if [ ! -z $(PCD_VERSION) ]; then \
		echo "#define PCD_VERSION            \"$(PCD_VERSION)\"" >> $(PCD_VERSION_H); \
		echo "#define PCD_VERSION_DATE       \"$(PCD_VERSION_DATE)\"" >> $(PCD_VERSION_H); \
		echo "#define PCD_BRANCH             \"$(PCD_BRANCH)\"" >> $(PCD_VERSION_H); \
		echo "#define PCD_BUILD_DATE         \"$(PCD_BUILD_DATE)\"" >> $(PCD_VERSION_H); \
	else \
		echo "#define PCD_VERSION            \"$(PCD_TAG_VERSION)\"" >> $(PCD_VERSION_H); \
		echo "#define PCD_VERSION_DATE       \"\"" >> $(PCD_VERSION_H); \
		echo "#define PCD_BRANCH             \"\"" >> $(PCD_VERSION_H); \
		echo "#define PCD_BUILD_DATE         \"$(PCD_BUILD_DATE)\"" >> $(PCD_VERSION_H); \
	fi
	@echo "" >> $(PCD_VERSION_H);
	@echo "#endif" >> $(PCD_VERSION_H);

