root := $(patsubst %/,%,$(dir $(lastword ${MAKEFILE_LIST})))
target ?= .
srcdir ?= ${root}
dstdir ?= bin
exesuf ?= $(and ${SYSTEMROOT},.exe)
program ?= main


.SUFFIXES:
.DELETE_ON_ERROR:


target := queue
srcdir := ${root}/${target}
include ${root}/${target}/Makefile

target := socket
srcdir := ${root}/${target}
include ${root}/${target}/Makefile


target := .
srcdir := ${root}


src :=
src += ${program}.c
src += ${program}_send.c
src += ${program}_recv.c
src += ${program}_play.c
src += gfx_v3.c
obj := ${src:%.c=${dstdir}/%.o}


.PHONY: ${target}/all
.PHONY: ${target}/clean
.PHONY: ${target}/run

.DEFAULT_GOAL := ${target}/all
${target}/all: ${dstdir}/${program}
${target}/clean: dstdir := ${dstdir}
${target}/clean: program := ${program}
${target}/clean::
	rm -rf ${dstdir}/${program}
	rm -rf ${dstdir}/*.o
	rm -rf ${dstdir}/*.a
	rm -rf ${dstdir}/*.exe

${dstdir}/${program}: ${dstdir}/libqueue.a
${dstdir}/${program}: ${dstdir}/libsocket.a
${dstdir}/${program}: override LDFLAGS += -lpthread
${dstdir}/${program}: override LDFLAGS += -lX11
${dstdir}/${program}: override LDFLAGS += -lm
${dstdir}/${program}: override LDFLAGS += $(and ${SYSTEMROOT},-lws2_32)
${dstdir}/${program}: ${obj} | ${dstdir}/
	$(strip \
		$(if $V,,@echo LD $@ && ) \
		${CC} -o $@ $(filter %.o %.a, $^) ${CFLAGS} ${LDFLAGS} \
	)

${target}/run: run := $(abspath ${dstdir}/${program})
${target}/run: ${dstdir}/${program}
	${run}


.INTERMEDIATE: ${obj}
${obj}: override CFLAGS += -I${srcdir}/queue
${obj}: override CFLAGS += -I${srcdir}/socket
${obj}: ${dstdir}/%.o: ${srcdir}/%.c ${srcdir}/videojuego.h | ${dstdir}/
	$(strip \
		$(if $V,,@echo CC $@ && ) \
		${CC} -c -o $@ $(filter %.c, $^) ${CFLAGS} \
	)


%/:
	$(if $(wildcard $@.),,mkdir $@)
