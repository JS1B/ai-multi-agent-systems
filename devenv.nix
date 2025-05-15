{ pkgs, lib, ... }:

{
  # languages.rust = {
  #   enable = true;
  #   # https://devenv.sh/reference/options/#languagesrustchannel
  #   channel = "nightly";
  #
  #   components = [ "rustc" "cargo" "clippy" "rustfmt" "rust-analyzer" ];
  # };
  #
  # languages.java = {
  #   enable = true;
  # };

  languages.cplusplus = {
    enable = true;
    # You can specify a compiler package if needed, e.g.:
    # package = pkgs.llvmPackages_latest.clang;
    # Or for GCC:
    # package = pkgs.gcc;
  };

  # git-hooks.hooks = {
  #   # Consider adding clang-format hook if desired
  #   # rustfmt.enable = true;
  #   # clippy.enable = true;
  # };

  packages = [
    pkgs.clang-tools # For clangd, clang-tidy, clang-format
    pkgs.cmake
    pkgs.gdb
    pkgs.gtest
    pkgs.linuxPackages.perf # Keeping this as it's useful
  ];
}