.DEFAULT_GOAL := all
.PHONY: all tidy tidy-tests format test test-verbose clean

all: test

test:
	@$(MAKE) -C tests test

test-verbose:
	@$(MAKE) -C tests test-verbose

clean:
	@$(MAKE) -C tests clean

CLANG_TIDY_RUN ?= run-clang-tidy
CMAKE_FLAGS ?= -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
# Exclude vendored Catch2 amalgamation from analysis
TIDY_FILES ?= '^(?!.*(/Catch2/catch_amalgamated\\.cpp|/Catch2/catch_amalgamated\\.hpp)).*$$'

tidy: tidy-tests tidy-sdl-ibmf tidy-sdl-ttf

tidy-tests:
	@mkdir -p tests/build
	@cmake -S tests -B tests/build $(CMAKE_FLAGS)
	@$(CLANG_TIDY_RUN) -p tests/build $(TIDY_FILES)

CLANG_FORMAT ?= clang-format
# Format all tracked C/C++ sources via git, excluding Catch2 amalgamation
FORMAT_FILES := $(shell git ls-files '*.c' '*.cc' '*.cxx' '*.cpp' '*.h' '*.hpp' | grep -Ev '^(components/|tests/Catch2/)')

format:
	@echo Formatting $$((`echo "$(FORMAT_FILES)" | wc -w`)) files...
	@echo "$(FORMAT_FILES)" | xargs -n 50 $(CLANG_FORMAT) -i
