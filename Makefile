PIN_ROOT ?= /afs/cs.cmu.edu/academic/class/15740-s17/public/pin-3.0
CONFIG_ROOT = $(PIN_ROOT)/source/tools/Config

include $(CONFIG_ROOT)/makefile.config
include $(PIN_ROOT)/source/tools/SimpleExamples/makefile.rules
include $(TOOLS_ROOT)/Config/makefile.default.rules