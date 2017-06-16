self := $(patsubst %/,%,$(dir $(lastword ${MAKEFILE_LIST})))
target ?= .
dstdir ?= $(or $(and ${SYSTEMROOT},win32),linux)
srcdir ?= ${self}
exesuf ?= $(and ${SYSTEMROOT},.exe)


.PHONY: ${target}/all
.PHONY: ${target}/test
.PHONY: ${target}/clean


hdr := ${srcdir}/socket.h
lib := ${dstdir}/libsocket.a

src :=
src += socket_addr.c
src += socket_creation.c
src += socket_init.c
src += socket_monitor.c
src += socket_options.c
src += socket_send_recv.c
obj := ${src:%.c=${dstdir}/%.o}

tst :=
tst += test_unicast_listener.c
tst += test_unicast_talker.c
tst += test_broadcast_talker.c
tst += test_multicast_listener.c
tstexe := ${tst:%.c=${dstdir}/%${exesuf}}


${target}/lib:  ${lib}
${target}/test: ${tstexe}
${target}/clean: files :=
${target}/clean: files += ${lib}
${target}/clean: files += ${obj}
${target}/clean: files += ${tstexe}
${target}/clean:
	rm -f ${files}


${lib}: ${obj}
	$(strip \
		$(if $V,,@echo AR $@ && ) \
		${AR} rcs $@ $(or $?, $^) \
	)


${obj}: ${hdr} ${srcdir}/_common.h
${obj}: ${dstdir}/%.o: ${srcdir}/%.c | ${dstdir}/
	$(strip \
		$(if $V,,@echo CC $@ && ) \
		${CC} -c -o $@ $(filter %.c, $^) ${CFLAGS} \
	)


${tstexe}: override LDFLAGS += $(and ${SYSTEMROOT},-lws2_32)
${tstexe}: ${dstdir}/%${exesuf}: ${srcdir}/%.c ${lib} | ${dstdir}/
	$(strip \
		$(if $V,,@echo LD $@ && ) \
		${CC} -o $@ $(filter %.c %.a, $^) ${CFLAGS} ${LDFLAGS} \
	)


$(if $(wildcard ${dstdir}/.), \
	, \
$(eval ${dstdir}/: ; \
	$$(if $$(wildcard $$@.),,mkdir $$@) \
) \
)
