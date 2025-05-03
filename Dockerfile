FROM nixos/nix:latest

# Enable flakes and increase cache size for better performance
RUN mkdir -p /etc/nix && \
    echo 'experimental-features = nix-command flakes' >> /etc/nix/nix.conf && \
    echo 'max-jobs = auto' >> /etc/nix/nix.conf && \
    echo 'cores = 0' >> /etc/nix/nix.conf && \
    echo 'substituters = https://cache.nixos.org https://nix-community.cachix.org' >> /etc/nix/nix.conf && \
    echo 'trusted-public-keys = cache.nixos.org-1:6NCHdD59X431o0gWypbMrAURkbJ16ZPMQFGspcDShjY= nix-community.cachix.org-1:mB9FSh9qf2dCimDSUo8Zy7bkq5CX+/rkCWyvRCYg3Fs=' >> /etc/nix/nix.conf

# Create simple entrypoint script - only sets kernel parameters
RUN printf '#!/bin/sh\n\
# Configure kernel parameters if we have permission\n\
if [ -w /proc/sys/kernel/perf_event_paranoid ]; then\n\
  echo "Setting kernel parameters for profiling..."\n\
  echo -1 > /proc/sys/kernel/perf_event_paranoid\n\
  echo 0 > /proc/sys/kernel/kptr_restrict\n\
  echo 65535 > /proc/sys/kernel/perf_event_mlock_kb\n\
  echo "Performance monitoring parameters set for profiling"\n\
fi\n\
\n\
# Enter nix develop\n\
cd /code\n\
exec nix --extra-experimental-features "nix-command flakes" develop\n\
' > /entrypoint.sh && \
    chmod +x /entrypoint.sh

# Copy flake files for pre-caching
COPY flake.nix flake.lock /tmp/nix-cache/

# Pre-build the devShell to cache dependencies
WORKDIR /tmp/nix-cache
RUN nix --extra-experimental-features "nix-command flakes" develop --command echo "Nix dependencies pre-built and cached"

# Set the final working directory
WORKDIR /code

# Very basic entrypoint approach
ENTRYPOINT ["/bin/sh", "/entrypoint.sh"] 