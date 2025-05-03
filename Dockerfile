FROM nixos/nix:latest

# Enable flakes
RUN mkdir -p /etc/nix && \
    echo 'experimental-features = nix-command flakes' > /etc/nix/nix.conf

# Create the code directory
RUN mkdir -p /code

# Set the working directory
WORKDIR /code

# The CMD now handles user ID mapping for better permissions
CMD bash -c 'export HOST_UID=$(stat -c "%u" /code) && \
    export HOST_GID=$(stat -c "%g" /code) && \
    echo "Host user ID: $HOST_UID, Group ID: $HOST_GID" && \
    echo "Setting kernel parameters if possible..." && \
    (echo -1 > /proc/sys/kernel/perf_event_paranoid 2>/dev/null || true) && \
    (echo 0 > /proc/sys/kernel/kptr_restrict 2>/dev/null || true) && \
    (echo 65535 > /proc/sys/kernel/perf_event_mlock_kb 2>/dev/null || true) && \
    cd /code && \
    echo "Starting Nix shell..." && \
    if [ $HOST_UID -ne 0 ]; then \
        echo "Running as non-root user for better permissions" && \
        addgroup --gid $HOST_GID nix_user || true && \
        adduser --disabled-password --gecos "" --uid $HOST_UID --gid $HOST_GID nix_user || true && \
        chown -R $HOST_UID:$HOST_GID /code && \
        # Allow the new user to use Nix
        mkdir -p /nix/var/nix/{profiles,gcroots}/per-user/nix_user && \
        chown -R nix_user:nix_user /nix/var/nix/{profiles,gcroots}/per-user/nix_user && \
        su nix_user -c "cd /code && nix develop"; \
    else \
        nix develop; \
    fi' 