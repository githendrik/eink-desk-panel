.PHONY: all render open clean venv

CXX = g++
CXXFLAGS = -std=c++11 -I. -I../include

all: sim42

# Compiles the REAL src/screen_42.h; only the EPD primitives are stubbed.
sim42: sim42.cpp data_stub.h EPD_GUI.h ../src/screen_42.h
	$(CXX) $(CXXFLAGS) -o sim42 sim42.cpp

# Resolve a python3 that actually has Pillow: prefer $PYTHON, else python3,
# else fall back to the local venv created by `make venv`.
PYTHON ?= $(shell \
	if python3 -c 'import PIL' >/dev/null 2>&1; then echo python3; \
	elif [ -x .venv/bin/python3 ]; then echo .venv/bin/python3; \
	else echo python3; fi)

render: sim42
	@$(PYTHON) -c 'import PIL' 2>/dev/null || { \
		echo ""; \
		echo "Pillow is missing for: $(PYTHON)"; \
		echo "Fix with either:"; \
		echo "  pip3 install -r requirements.txt"; \
		echo "  make venv        (creates ./.venv with Pillow)"; \
		echo ""; \
		exit 1; }
	./sim42
	$(PYTHON) render_42.py sim42_preview.png

venv:
	python3 -m venv .venv
	.venv/bin/pip install -q -r requirements.txt
	@echo "Created ./.venv - now run: make open"

open: render
	open sim42_preview.png

clean:
	rm -rf .venv
	rm -f sim42 sim42.pgm sim42_text.txt sim42_preview.png
