# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the bfq kernel module
# Author: Kuan
#

# EXTRA_CFLAGS += -fplugin=/root/adelie-kaslr-linux-5.0.4/source-main/gcc-plugins/fix_relocations_plugin.so
# EXTRA_CFLAGS += -fplugin=/root/adelie-kaslr-linux-5.0.4/source-main/gcc-plugins/function_proepilogue_plugin.so
# EXTRA_CFLAGS += -fplugin=/root/adelie-kaslr-linux-5.0.4/source-main/gcc-plugins/rerandomization_wrapper_plugin.so
EXTRA_CFLAGS += -ffunction-sections
bfq-y				:= bfq-iosched.o bfq-wf2q.o bfq-cgroup.o
obj-$(CONFIG_IOSCHED_BFQ)	+= bfq.o