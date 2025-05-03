{
  description = "C/C++ development environment";

  inputs.nixpkgs.url = "https://flakehub.com/f/NixOS/nixpkgs/0.1.*.tar.gz";

  outputs = { self, nixpkgs }:
    let
      supportedSystems = [ "x86_64-linux" ];
      forEachSupportedSystem = f: nixpkgs.lib.genAttrs supportedSystems (system: f {
        pkgs = import nixpkgs { inherit system; };
      });
    in
    {
      packages = forEachSupportedSystem ({ pkgs }: {
        default = pkgs.dockerTools.buildImage {
          name = "cpp-dev-image";
          tag = "latest";
          
          contents = with pkgs; [
            bashInteractive
            coreutils
            clang-tools
            cmake
            gtest
            gdb
          ];
          
          config = {
            Cmd = [ "${pkgs.bashInteractive}/bin/bash" ];
            WorkingDir = "/code";
          };
        };
      });
    };
}

