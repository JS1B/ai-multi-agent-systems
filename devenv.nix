{ pkgs, lib, ... }:

{
  languages.rust = {
    enable = true;
    # https://devenv.sh/reference/options/#languagesrustchannel
    channel = "nightly";

    components = [ "rustc" "cargo" "clippy" "rustfmt" "rust-analyzer" ];
  };

  languages.java = {
    enable = true;
  };

  # git-hooks.hooks = {
  #   rustfmt.enable = true;
  #   clippy.enable = true;
  # };

  packages = [
    pkgs.linuxPackages.perf
  ];
}