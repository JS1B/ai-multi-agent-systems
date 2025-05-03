FROM nixos/nix:latest

# Create and set up working directory
WORKDIR /code

# Copy flake files
COPY flake.nix flake.lock ./
RUN mkdir -p ~/.config/nix && \
    echo 'experimental-features = nix-command flakes' >> ~/.config/nix/nix.conf

# Use the flake's shell environment
ENTRYPOINT ["nix", "develop"]

# Start bash by default
CMD ["bash"] 