{
  inputs.nixpkgs.url = "github:nixos/nixpkgs";

  outputs =
    { nixpkgs, ... }:
    let
      system = "x86_64-linux";
      pkgs = nixpkgs.legacyPackages.${system};
    in
    {
      devShells.${system}.default = pkgs.mkShell {
        packages = with pkgs; [
          pkg-config
          (raylib.overrideAttrs (
            self: {
              cmakeFlags = self.cmakeFlags ++ [ "-DSUPPORT_FILEFORMAT_JPG=ON" ];
            }
          ))
          bear
        ];
      };
    };
}
