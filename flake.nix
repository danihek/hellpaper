{
  inputs.nixpkgs.url = "github:nixos/nixpkgs";

  outputs =
    { nixpkgs, ... }:
    let
      system = "x86_64-linux";
      pkgs = nixpkgs.legacyPackages.${system};
      raylibWithJPG = pkgs.raylib.overrideAttrs (finalAttrs: {
        cmakeFlags = finalAttrs.cmakeFlags ++ [ "-DSUPPORT_FILEFORMAT_JPG=ON" ];
      });
    in
    {
      devShells.${system}.default = pkgs.mkShell {
        packages = [
          raylibWithJPG
          pkgs.pkg-config # For local compilation + linking with raylib
          pkgs.bear # For generating compile_commands.json (used by LSPs)
        ];
      };

      packages.${system} = rec {
        hellpaper = pkgs.stdenv.mkDerivation {
          pname = "hellpaper";
          version = "1.0.0";
          src = ./.;

          nativeBuildInputs = [ raylibWithJPG ];

          buildPhase = ''
            make
          '';

          installPhase = ''
            mkdir -p $out/bin
            mv ./hellpaper $out/bin/
          '';

          meta.mainProgram = "hellpaper";
        };

        default = hellpaper;
      };
    };
}
