{
  pkgs ? import <nixpkgs> { },
  unstable ? import <nixos-unstable> { },
}:
pkgs.mkShell {
  nativeBuildInputs = with pkgs.buildPackages; [
    nodejs_22
    gnumake
    gcc14
    llvmPackages_19.clang-tools
    llvmPackages_19.libcxx
    clang_19
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
