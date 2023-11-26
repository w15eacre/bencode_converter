FROM mcr.microsoft.com/vscode/devcontainers/cpp:0-jammy

RUN sudo apt-get update; \
	sudo apt-get install -y \
		libgtest-dev \
        libfmt-dev \
    && sudo rm -rf /var/lib/apt/lists/*;
