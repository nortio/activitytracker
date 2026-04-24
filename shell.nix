{
  pkgs ? import <nixpkgs> { },
  unstable ? import <nixos-unstable> { },
}:
pkgs.mkShell {
  nativeBuildInputs = with pkgs.buildPackages; [
    nodejs_24
    gnumake
    clang-tools
    kdePackages.kcachegrind
    valgrind
    bear
    #llvmPackages_19.libcxx.dev
    boost.dev
    (pkgs.python3.withPackages (
      python-pkgs: with python-pkgs; [
        numpy
        matplotlib
      ]
    ))
    sdl3
    pkgconf
  ];
}
