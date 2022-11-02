# builds and checks project
all:
	@echo "build and test"
	cd pico/build; make
	@echo "reuse (license) check"
	reuse lint

# rebuild
rebuild:
	@echo "rebuild"
	rm -rf pico/build
	mkdir pico/build
	cd pico/build; cmake .. -DPICO_BOARD=pico_w; make

#install fsfe reuse tool (https://git.fsfe.org/reuse/tool)
# pre-conditions:
# - Python 3.6+
# - pip
# install pre-conditions in Debian like linux distros:
# - sudo apt install python3
# - sudo apt install python3-pip
reuse:
	pip3 install --user --upgrade reuse
