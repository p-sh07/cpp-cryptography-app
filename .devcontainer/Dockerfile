# Base image
# Date: 2025-01-27
FROM ubuntu@sha256:3afff29dffbc200d202546dc6c4f614edc3b109691e7ab4aa23d02b42ba86790

# Update the package database and install required tools
RUN apt-get update
RUN apt-get install -y python3.12 python3-pip pipx git less vim sudo cmake make g++-14 clangd-19 libjemalloc-dev

# Update clangd and g++ versions to the latest ones
RUN ln -s  /usr/bin/clangd-19 /usr/bin/clangd
RUN ln -fs /usr/bin/g++-14 /usr/bin/g++

# Create a non-root user 'dev'
RUN useradd -ms /bin/bash dev \
    && echo "dev ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers.d/dev \
    && chmod 0440 /etc/sudoers.d/dev

# Switch to the 'dev' user
USER dev

RUN git config --global core.editor code

# Install conan through pipx
RUN pipx install conan
RUN pipx ensurepath

# Copy conan profile
COPY default /home/dev/.conan2/profiles/default
RUN sudo chown -R dev:dev /home/dev/.conan2

# Set the default command
CMD ["/bin/bash"]
