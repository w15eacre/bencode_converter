FROM ubuntu:22.04

RUN set -ex; \
	sudo apt-get update; \
	sudo apt-get install -y \
		libgtest-dev \
        libfmt-dev \
    && sudo rm -rf /var/lib/apt/lists/*;
