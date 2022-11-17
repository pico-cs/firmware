# builds and checks project
all:
	@echo "build pico"
	mkdir -p src/pico_build
	cd src/pico_build; make
	@echo "build pico_w"
	mkdir -p src/pico_w_build
	cd src/pico_w_build; make
	@echo "reuse (license) check"
	reuse lint

# rebuild
rebuild:
	@echo "rebuild pico"
	rm -rf src/pico_build
	mkdir src/pico_build
	cd src/pico_build; cmake .. -DPICO_BOARD=pico; make
	@echo "rebuild pico_w"
	rm -rf src/pico_w_build
	mkdir src/pico_w_build
	cd src/pico_w_build; cmake .. -DPICO_BOARD=pico_w; make

#install fsfe reuse tool (https://git.fsfe.org/reuse/tool)
# pre-conditions:
# - Python 3.6+
# - pip
# install pre-conditions in Debian like linux distros:
# - sudo apt install python3
# - sudo apt install python3-pip
reuse:
	pip3 install --user --upgrade reuse
