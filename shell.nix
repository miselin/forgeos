let pkgs = import <nixpkgs> {
  crossSystem = (import <nixpkgs/lib>).systems.examples.i686-embedded;
};
in
  pkgs.callPackage (
    {mkShell}:
    mkShell {
      nativeBuildInputs = [ ];
      buildInputs = [ ];
    }
  ) {}