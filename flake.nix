{
  description = "Dev env for ESP IDF, with SDL2 for desktop LVGL";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=8a6d5427d99ec71c64f0b93d45778c889005d9c2";
  };

  outputs = {nixpkgs, ...}: let
    pkgs = import nixpkgs {system = "aarch64-darwin";};
  in {
    devShells."aarch64-darwin".default = pkgs.mkShell {
      name = "sdl2-project";

      buildInputs = with pkgs; [
        SDL2.dev
        apple-sdk_15
      ];
    };
  };
}
