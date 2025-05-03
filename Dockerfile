FROM nixos/nix:latest

# Enable flakes
RUN mkdir -p /etc/nix && \
    echo 'experimental-features = nix-command flakes' > /etc/nix/nix.conf

# Create the code directory
RUN mkdir -p /code

# Set the working directory
WORKDIR /code

# Super simple - just use a direct bash command as the CMD
# No script files that might cause path issues
CMD bash -c 'echo "Setting kernel parameters if possible..."; \
    echo -1 > /proc/sys/kernel/perf_event_paranoid 2>/dev/null || true; \
    echo 0 > /proc/sys/kernel/kptr_restrict 2>/dev/null || true; \
    echo 65535 > /proc/sys/kernel/perf_event_mlock_kb 2>/dev/null || true; \
    cd /code; \
    echo "Starting Nix shell..."; \
    nix develop' 