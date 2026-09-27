FROM mcr.microsoft.com/devcontainers/base:trixie

ARG INSTALL_LATEX=false

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        ninja-build \
        gdb \
        valgrind \
        strace \
        clang-format \
        clang-tidy \
        clangd \
        gcovr \
        libgtest-dev \
        python3 \
        python3-venv \
        pre-commit \
        nodejs \
        npm \
    && if [ "$INSTALL_LATEX" = "true" ]; then \
        apt-get install -y --no-install-recommends \
            texlive-latex-base \
            texlive-latex-recommended \
            texlive-latex-extra \
            texlive-fonts-recommended \
            lmodern \
            latexmk \
            chktex; \
    fi \
    && rm -rf /var/lib/apt/lists/* \
    && git config --system --add safe.directory '*'
