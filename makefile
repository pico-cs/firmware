# builds and checks project
all:
	@echo "build and test"
	cd firmware/build; cmake ..; make
	@echo "reuse (license) check"
	reuse lint

# rebuild
rebuild:
	@echo "rebuild"
	rm -rf firmware/build
	mkdir firmware/build
	cd firmware/build; cmake ..; make

#install fsfe reuse tool (https://git.fsfe.org/reuse/tool)
# pre-conditions:
# - Python 3.6+
# - pip
# install pre-conditions in Debian like linux distros:
# - sudo apt install python3
# - sudo apt install python3-pip
reuse:
	pip3 install --user --upgrade reuse
