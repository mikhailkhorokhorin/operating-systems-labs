PRESET ?= debug
LAB ?=
APP ?= main
ARGS ?=
COVERAGE_MIN ?= 80

BUILD_DIR := build/$(PRESET)
CI_EXTRA :=

PRE_COMMIT_CONFIG := .config/.pre-commit-config.yaml

-include .config/local.mk

.DEFAULT_GOAL := help

.PHONY: help setup configure build test run format lint tidy sanitize coverage ci ci-build clean

help:
	@echo "make build    [PRESET=debug] [LAB=labN]   configure and build"
	@echo "make test     [PRESET=debug] [LAB=labN]   build and run tests"
	@echo "make run      LAB=labN [APP=main] [ARGS=]  run labN_APP"
	@echo "make format                               apply all formatters"
	@echo "make lint                                 pre-commit hooks and clang-tidy"
	@echo "make sanitize                             tests under ASan/UBSan and TSan"
	@echo "make coverage [COVERAGE_MIN=80]           line coverage gate"
	@echo "make ci                                   everything CI runs"
	@echo "make clean                                remove build outputs"

setup:
	pre-commit install -c $(PRE_COMMIT_CONFIG)

configure:
	cmake --preset $(PRESET)

build: configure
	cmake --build --preset $(PRESET) $(if $(LAB),--target $(LAB))

test: build
	ctest --preset $(PRESET) $(if $(LAB),-R '^$(LAB)\.')

run:
	@test -n "$(LAB)" || (echo "LAB is required" >&2; exit 2)
	@mkdir -p $(BUILD_DIR)
	@cmake --preset $(PRESET) > $(BUILD_DIR)/run.log 2>&1 \
		&& cmake --build --preset $(PRESET) --target $(LAB) >> $(BUILD_DIR)/run.log 2>&1 \
		|| (cat $(BUILD_DIR)/run.log >&2; exit 1)
	@$(BUILD_DIR)/$(LAB)/$(LAB)_$(APP) $(ARGS)

format:
	-pre-commit run -c $(PRE_COMMIT_CONFIG) --all-files

lint:
	pre-commit run -c $(PRE_COMMIT_CONFIG) --all-files --show-diff-on-failure
	$(MAKE) tidy

tidy:
	cmake --preset debug
	run-clang-tidy -quiet -p build/debug \
		-header-filter '^$(CURDIR)/.*/include/.*' \
		'^$(CURDIR)/.*/(src|app)/[^/]*\.cpp$$'

sanitize:
	cmake --preset asan
	cmake --build --preset asan
	ctest --preset asan
	cmake --preset tsan
	cmake --build --preset tsan
	setarch $$(uname -m) -R ctest --preset tsan -L threads

coverage:
	cmake --preset coverage
	cmake --build --preset coverage
	find build/coverage -name '*.gcda' -delete
	ctest --preset coverage
	mkdir -p build/artifacts/coverage
	gcovr --root . --object-directory build/coverage \
		--filter '$(CURDIR)/(lab[0-9]+|coursework)/(src|include)/' \
		--txt --html-details build/artifacts/coverage/index.html \
		--fail-under-line $(COVERAGE_MIN)

ci-build:
	cmake --preset ci
	cmake --build --preset ci
	ctest --preset ci

ci: lint ci-build sanitize coverage $(CI_EXTRA)

clean:
	rm -rf build
