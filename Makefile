# builds and checks project
all:
	@echo "build pico"
	mkdir -p src/pico_build
	cd src/pico_build; make -j4
	@echo "build pico_w"
	mkdir -p src/pico_w_build
	cd src/pico_w_build; make -
#see fsfe reuse tool (https://git.fsfe.org/reuse/tool)
	@echo "reuse (license) check"
	pipx run reuse lint

# rebuild
rebuild:
	@echo "rebuild pico"
	rm -rf src/pico_build
	mkdir src/pico_build
	cd src/pico_build; cmake .. -DPICO_BOARD=pico; make -j4
	@echo "rebuild pico_w"
	rm -rf src/pico_w_build
	mkdir src/pico_w_build
	cd src/pico_w_build; cmake .. -DPICO_BOARD=pico_w; make -j4
