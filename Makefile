PACKAGE_NAME ?= mjsxj06cm_onvif
PACKAGE_VERSION ?= $(shell date +%Y%m%d_%H%M%S)
PACKAGE_FILE ?= dist/$(PACKAGE_NAME)-$(PACKAGE_VERSION).tar.gz
PACKAGE_ROOT := build/package/$(PACKAGE_NAME)
BUSYBOX ?= busybox

.PHONY: all rtsp onvif wsdd package clean-package clean

all: rtsp onvif wsdd

rtsp:
	$(MAKE) -C rtsp_demo
	mkdir -p bin
	cp -f rtsp_demo/mi_rtsp_server bin/mi_rtsp_server

onvif:
	$(MAKE) -C onvif_srvd release

wsdd:
	$(MAKE) -C onvif_wsdd

package: all
	rm -rf build/package
	mkdir -p "$(PACKAGE_ROOT)/bin" "$(PACKAGE_ROOT)/lib" "$(PACKAGE_ROOT)/onvif_srvd" "$(PACKAGE_ROOT)/onvif_wsdd" "$(PACKAGE_ROOT)/scripts" dist
	cp -a bin/. "$(PACKAGE_ROOT)/bin/"
	cp -aL lib/libmortox.so.0.0.0 "$(PACKAGE_ROOT)/lib/"
	cp -a onvif_srvd/onvif_srvd onvif_srvd/mi_ptz_control "$(PACKAGE_ROOT)/onvif_srvd/"
	cp -a onvif_wsdd/onvif_wsdd "$(PACKAGE_ROOT)/onvif_wsdd/"
	cp -a scripts/start_onvif_stack.sh scripts/stop_onvif_stack.sh "$(PACKAGE_ROOT)/scripts/"
	cp -a README.md ARCHITECTURE.md "$(PACKAGE_ROOT)/"
	$(BUSYBOX) tar -czf "$(PACKAGE_FILE)" -C build/package "$(PACKAGE_NAME)"
	@echo "package: $(PACKAGE_FILE)"

clean-package:
	rm -rf build/package

clean:
	$(MAKE) -C rtsp_demo clean
	$(MAKE) -C onvif_srvd clean
	$(MAKE) -C onvif_wsdd clean
	rm -rf build/package
