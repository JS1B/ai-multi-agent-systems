# Use the official Nix image
FROM nixos/nix:latest

# Set the working directory
WORKDIR /workspace

# Enable Nix sandboxing (requires running container with --privileged)
# See: https://hub.docker.com/r/nixos/nix
RUN echo "sandbox = true" >> /etc/nix/nix.conf && echo "experimental-features = nix-command flakes" >> /etc/nix/nix.conf

# Copy flake files first for better caching
COPY flake.nix flake.lock* ./

# Copy the rest of the application code
COPY . .

# Add cargo bin directory to PATH
ENV PATH="/root/.cargo/bin:${PATH}"

# Set the default command to enter the Nix development shell
CMD ["nix", "develop", "--command", "bash"]