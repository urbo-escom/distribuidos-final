self := $(patsubst %/,%,$(dir $(lastword ${MAKEFILE_LIST})))
target ?= .
dstdir ?= .
srcdir ?= ${self}
exe_suf ?= $(and ${SYSTEMROOT},.exe)


lib := libqueue.a

src :=
src += queue.c
obj := ${src:%.c=${dstdir}/%.o}

tst :=
tst += test_queue.c
tstexe := ${tst:%.c=${dstdir}/%${exe_suf}}


.PHONY: ${target}/lib
.PHONY: ${target}/test


${target}/lib: ${dstdir}/${lib}
${target}/test: ${tstexe}


${dstdir}/${lib}: ${obj}
	$(strip \
		$(if $V,,@echo AR $@ && ) \
		${AR} rcs $@ $(or $?, $^) \
	)


.INTERMEDIATE: ${obj}
${obj}: ${dstdir}/%.o: ${srcdir}/%.c
	$(strip \
		$(if $V,,@echo CC $@ && ) \
		${CC} -c -o $@ $(filter %.c, $^) ${CFLAGS} \
	)


${tstexe}: override LDFLAGS += -lpthread
${tstexe}: ${dstdir}/%${exe_suf}: \
		${srcdir}/%.c ${dstdir}/${lib}
	$(strip \
		$(if $V,,@echo LD $@ && ) \
		${CC} -o $@ $(filter %.c %.a, $^) ${CFLAGS} ${LDFLAGS} \
	)
