{pkgs}:

let

  pkgs = import <nixpkgs> {
    crossSystem = (import <nixpkgs/lib>).systems.examples.i686-embedded;
  };

in
  pkgs.callPackage (
    {mkShell, gnumake, python3, cdrkit, qemu}:
    mkShell {
      depsBuildBuild = [
        gnumake
        python3
        cdrkit
        qemu
      ];
      buildInputs = [
      ];
    }
  ) {}