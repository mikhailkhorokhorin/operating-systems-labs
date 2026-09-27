TRACE_PRESET ?= release
TRACE_BUILD := build/$(TRACE_PRESET)
TRACE_LABS := lab1 lab2 lab3 lab4
STRACE := strace -f

.PHONY: trace trace-build $(addprefix trace-,$(TRACE_LABS))

define clean-trace
sed -i -e 's|$(CURDIR)/||g' -e 's/[[:space:]]*$$//' $(1)
endef

trace: $(if $(LAB),trace-$(LAB),$(addprefix trace-,$(TRACE_LABS)))

trace-build:
	cmake --preset $(TRACE_PRESET)
	cmake --build --preset $(TRACE_PRESET)

trace-lab1: trace-build
	echo lab1/tests/data/01.in | $(STRACE) -o lab1/docs/strace.log \
		$(TRACE_BUILD)/lab1/lab1_main > /dev/null
	$(call clean-trace,lab1/docs/strace.log)

trace-lab2: trace-build
	$(STRACE) -o lab2/docs/strace.log $(TRACE_BUILD)/lab2/lab2_main 2 \
		< lab2/tests/data/01.in > /dev/null
	$(call clean-trace,lab2/docs/strace.log)

trace-lab3: trace-build
	echo lab3/tests/data/01.in | $(STRACE) -o lab3/docs/strace.log \
		$(TRACE_BUILD)/lab3/lab3_main > /dev/null
	$(call clean-trace,lab3/docs/strace.log)

trace-lab4: trace-build
	$(STRACE) -o lab4/docs/strace-static.log $(TRACE_BUILD)/lab4/lab4_static \
		< lab4/tests/data/static/01.in > /dev/null 2>&1
	$(STRACE) -o lab4/docs/strace-dynamic.log $(TRACE_BUILD)/lab4/lab4_dynamic \
		< lab4/tests/data/dynamic/01.in > /dev/null
	$(call clean-trace,lab4/docs/strace-static.log lab4/docs/strace-dynamic.log)
