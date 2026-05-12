.PHONY: all render clean open

CXX = g++
CXXFLAGS = -std=c++11 -I.

all: weather_sim

weather_sim: weather_screen_mock.cpp epd_mock.h
	$(CXX) $(CXXFLAGS) -o weather_sim weather_screen_mock.cpp

render: weather_sim
	./weather_sim display_data.json
	python3 render.py display_data.json display_preview.png

open: render
	open display_preview.png

clean:
	rm -f weather_sim display_data.json display_preview.png

help:
	@echo "E-Ink Desk Panel Simulator"
	@echo "=========================="
	@echo ""
	@echo "Targets:"
	@echo "  make          - Build the simulator"
	@echo "  make render   - Build, run, and generate PNG"
	@echo "  make open     - Build, render, and open in Preview"
	@echo "  make clean    - Remove generated files"
	@echo ""
	@echo "Quick start:"
	@echo "  make open"
