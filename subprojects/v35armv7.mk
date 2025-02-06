# This file is autogenerated by acr-wrap

WRAP_wrap_git_url:=https://github.com/radareorg/vector35-arch-armv7
WRAP_wrap_git_revision:=radare2-2024
WRAP_wrap_git_directory:=v35armv7
WRAP_wrap_git_depth:=1

v35armv7_all: v35armv7
	@echo "Nothing to do"

v35armv7:
	git clone --no-checkout --depth=1 https://github.com/radareorg/vector35-arch-armv7 v35armv7
	cd v35armv7 && git fetch --depth=1 origin radare2-2024
	cd v35armv7 && git checkout FETCH_HEAD

v35armv7_clean:
	rm -rf v35armv7
